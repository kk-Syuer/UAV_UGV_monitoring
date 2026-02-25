#!/usr/bin/env python3
import ast
import json
import re
from pathlib import Path
from collections import defaultdict

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
ART = ROOT / "artifacts"
OUT = ART / "ros_interfaces.json"

CPP_GLOB = list(SRC.glob("*/src/*.cpp"))
PY_GLOB = list(SRC.glob("*/**/*.py"))


def read(p: Path) -> str:
    return p.read_text(encoding="utf-8", errors="ignore")


def extract_cpp_node_name(text: str):
    m = re.search(r"Node\s*\(\s*\"([^\"]+)\"", text)
    return m.group(1) if m else None


def split_first_arg(arg_blob: str):
    depth = 0
    in_str = False
    esc = False
    quote = ''
    out = []
    for ch in arg_blob:
        if in_str:
            out.append(ch)
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == quote:
                in_str = False
            continue
        if ch in ('\"', "'"):
            in_str = True
            quote = ch
            out.append(ch)
            continue
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth = max(depth - 1, 0)
        if ch == "," and depth == 0:
            break
        out.append(ch)
    return "".join(out).strip()


def norm_name(expr: str):
    expr = expr.strip()
    if expr.startswith('"') and expr.endswith('"') and "+" not in expr:
        return expr[1:-1], False
    if expr.startswith("'") and expr.endswith("'") and "+" not in expr:
        return expr[1:-1], False
    return expr, True


def capture_cpp_interfaces(text: str):
    pubs, subs, srv_servers, srv_clients, act_servers, act_clients, timers = [], [], [], [], [], [], []

    def grab(kind, bucket):
        pat = re.compile(rf"{kind}\s*<\s*([^>]+?)\s*>\s*\((.*?)\)\s*;", re.S)
        for m in pat.finditer(text):
            mtype = re.sub(r"\s+", "", m.group(1))
            first = split_first_arg(m.group(2))
            name, dynamic = norm_name(first)
            bucket.append({"name": name, "type": mtype, "dynamic": dynamic})

    grab("create_publisher", pubs)
    grab("create_subscription", subs)
    grab("create_service", srv_servers)
    grab("create_client", srv_clients)

    # action best effort
    for m in re.finditer(r"rclcpp_action::create_server\s*<\s*([^>]+?)\s*>\s*\((.*?)\)\s*;", text, re.S):
        mtype = re.sub(r"\s+", "", m.group(1))
        args = m.group(2)
        pieces = [p.strip() for p in args.split(",")]
        name_expr = pieces[1] if len(pieces) > 1 else "<unknown>"
        name, dynamic = norm_name(name_expr)
        act_servers.append({"name": name, "type": mtype, "dynamic": dynamic})
    for m in re.finditer(r"rclcpp_action::create_client\s*<\s*([^>]+?)\s*>\s*\((.*?)\)\s*;", text, re.S):
        mtype = re.sub(r"\s+", "", m.group(1))
        args = m.group(2)
        pieces = [p.strip() for p in args.split(",")]
        name_expr = pieces[1] if len(pieces) > 1 else "<unknown>"
        name, dynamic = norm_name(name_expr)
        act_clients.append({"name": name, "type": mtype, "dynamic": dynamic})

    for m in re.finditer(r"create_wall_timer\s*\((.*?)\)\s*;", text, re.S):
        blob = m.group(1)
        cb = "<callback>"
        mm = re.search(r"&[\w:]+::([\w_]+)", blob)
        if mm:
            cb = mm.group(1)
        period = re.sub(r"\s+", " ", split_first_arg(blob))
        timers.append({"period_expr": period, "callback": cb})

    return pubs, subs, srv_servers, srv_clients, act_servers, act_clients, timers


def capture_python_interfaces(text: str):
    pubs, subs, srv_servers, srv_clients, act_servers, act_clients, timers = [], [], [], [], [], [], []

    def py_grab(fn, bucket, name_idx=1):
        pat = re.compile(rf"{fn}\s*\((.*?)\)", re.S)
        for m in pat.finditer(text):
            args = m.group(1)
            parts = [p.strip() for p in args.split(",")]
            if len(parts) <= name_idx:
                continue
            mtype = parts[0]
            name_expr = parts[name_idx]
            name, dynamic = norm_name(name_expr)
            bucket.append({"name": name, "type": mtype, "dynamic": dynamic})

    py_grab("create_publisher", pubs)
    py_grab("create_subscription", subs)
    py_grab("create_service", srv_servers)
    py_grab("create_client", srv_clients)

    for m in re.finditer(r"ActionServer\s*\((.*?)\)", text, re.S):
        parts = [p.strip() for p in m.group(1).split(",")]
        if len(parts) >= 3:
            name, dynamic = norm_name(parts[2])
            act_servers.append({"name": name, "type": parts[1], "dynamic": dynamic})
    for m in re.finditer(r"ActionClient\s*\((.*?)\)", text, re.S):
        parts = [p.strip() for p in m.group(1).split(",")]
        if len(parts) >= 3:
            name, dynamic = norm_name(parts[2])
            act_clients.append({"name": name, "type": parts[1], "dynamic": dynamic})

    for m in re.finditer(r"create_timer\s*\((.*?)\)", text, re.S):
        parts = [p.strip() for p in m.group(1).split(",")]
        if len(parts) >= 2:
            timers.append({"period_expr": parts[0], "callback": parts[1]})

    return pubs, subs, srv_servers, srv_clients, act_servers, act_clients, timers


def parse_cmake_execs(cmake: Path):
    txt = read(cmake)
    out = []
    for m in re.finditer(r"add_executable\(([^\s\)]+)\s+([^\)]+)\)", txt):
        exe = m.group(1).strip()
        srcs = [s.strip() for s in m.group(2).split()]
        out.append((exe, srcs))
    return out


def parse_setup_entrypoints(setup_py: Path):
    txt = read(setup_py)
    out = []
    for m in re.finditer(r"'([^']+?)\s*=\s*([^:]+):main'", txt):
        exe = m.group(1).strip()
        module = m.group(2).strip().replace(".", "/") + ".py"
        out.append((exe, [module]))
    return out


def parse_launch_nodes(launch_path: Path):
    txt = read(launch_path)
    nodes = []
    for m in re.finditer(r"Node\((.*?)\)\s*", txt, re.S):
        blob = m.group(1)
        pkg = re.search(r"package\s*=\s*([^,\n]+)", blob)
        exe = re.search(r"executable\s*=\s*([^,\n]+)", blob)
        name = re.search(r"name\s*=\s*([^,\n]+)", blob)
        nodes.append({
            "package_expr": pkg.group(1).strip() if pkg else "",
            "executable_expr": exe.group(1).strip() if exe else "",
            "name_expr": name.group(1).strip() if name else "",
        })
    return nodes


def parse_run_configs():
    runs = {}
    for p in sorted((SRC / "system_bringup/config/runs").glob("*.yaml")):
        txt = read(p)

        def pick(section, key):
            m = re.search(rf"^\s*{re.escape(section)}:\s*$([\s\S]*?)(?=^\S|\Z)", txt, re.M)
            if not m:
                return None
            block = m.group(1)
            km = re.search(rf"^\s*{re.escape(key)}:\s*([^\n#]+)", block, re.M)
            if not km:
                return None
            v = km.group(1).strip().strip("\"'")
            if v.lower() in ("true", "false"):
                return v.lower() == "true"
            return v

        exec_block = {}
        mexec = re.search(r"^\s*executables:\s*$([\s\S]*?)(?=^\S|\Z)", txt, re.M)
        if mexec:
            for ln in mexec.group(1).splitlines():
                kv = re.match(r"^\s*([\w_]+):\s*([^\n#]+)", ln)
                if kv:
                    exec_block[kv.group(1)] = kv.group(2).strip().strip("\"'")

        runs[p.name] = {
            "executables": exec_block,
            "enables": {
                "weather.enable": pick("weather", "enable"),
                "fault_injector.enable": pick("fault_injector", "enable"),
                "traffic.user_device_enable": pick("traffic", "user_device_enable"),
                "recovery_manager.enable": pick("recovery_manager", "enable"),
                "coverage_planner.enable": pick("coverage_planner", "enable"),
                "ch_manager.enable": pick("ch_manager", "enable"),
                "planner_viz.enable": pick("planner_viz", "enable"),
            },
        }
    return runs


def main():
    ART.mkdir(parents=True, exist_ok=True)
    nodes = []
    executable_index = {}

    for pkg_dir in sorted(SRC.iterdir()):
        if not pkg_dir.is_dir():
            continue
        pkg = pkg_dir.name
        cmake = pkg_dir / "CMakeLists.txt"
        setup_py = pkg_dir / "setup.py"
        exec_defs = []
        if cmake.exists():
            exec_defs.extend(parse_cmake_execs(cmake))
        if setup_py.exists():
            exec_defs.extend(parse_setup_entrypoints(setup_py))

        for exe, srcs in exec_defs:
            entry = {
                "package": pkg,
                "executable": exe,
                "sources": srcs,
                "node_name_hint": None,
                "publishers": [],
                "subscribers": [],
                "service_servers": [],
                "service_clients": [],
                "action_servers": [],
                "action_clients": [],
                "timers": [],
            }
            for src_rel in srcs:
                src_path = pkg_dir / src_rel
                if not src_path.exists():
                    src_path = pkg_dir / src_rel.replace(f"{pkg}/", "")
                if not src_path.exists():
                    continue
                txt = read(src_path)
                if src_path.suffix == ".cpp":
                    entry["node_name_hint"] = entry["node_name_hint"] or extract_cpp_node_name(txt)
                    pubs, subs, ss, sc, acs, acc, t = capture_cpp_interfaces(txt)
                else:
                    m = re.search(r"super\(\).__init__\(([^\)]+)\)", txt)
                    if m and not entry["node_name_hint"]:
                        nm, _ = norm_name(m.group(1).strip())
                        entry["node_name_hint"] = nm
                    pubs, subs, ss, sc, acs, acc, t = capture_python_interfaces(txt)
                entry["publishers"].extend(pubs)
                entry["subscribers"].extend(subs)
                entry["service_servers"].extend(ss)
                entry["service_clients"].extend(sc)
                entry["action_servers"].extend(acs)
                entry["action_clients"].extend(acc)
                entry["timers"].extend(t)
            nodes.append(entry)
            executable_index[f"{pkg}/{exe}"] = entry

    launch_nodes = parse_launch_nodes(SRC / "system_bringup/launch/experiment.launch.py")

    topic_map = defaultdict(lambda: {"type": None, "publishers": [], "subscribers": []})
    service_map = defaultdict(lambda: {"type": None, "servers": [], "clients": []})
    action_map = defaultdict(lambda: {"type": None, "servers": [], "clients": []})

    for n in nodes:
        node_id = f"{n['package']}/{n['executable']}"
        for p in n["publishers"]:
            t = topic_map[p["name"]]
            t["type"] = t["type"] or p["type"]
            t["publishers"].append(node_id)
        for s in n["subscribers"]:
            t = topic_map[s["name"]]
            t["type"] = t["type"] or s["type"]
            t["subscribers"].append(node_id)
        for p in n["service_servers"]:
            t = service_map[p["name"]]
            t["type"] = t["type"] or p["type"]
            t["servers"].append(node_id)
        for s in n["service_clients"]:
            t = service_map[s["name"]]
            t["type"] = t["type"] or s["type"]
            t["clients"].append(node_id)
        for p in n["action_servers"]:
            t = action_map[p["name"]]
            t["type"] = t["type"] or p["type"]
            t["servers"].append(node_id)
        for s in n["action_clients"]:
            t = action_map[s["name"]]
            t["type"] = t["type"] or s["type"]
            t["clients"].append(node_id)

    out = {
        "nodes": nodes,
        "topic_inventory": dict(topic_map),
        "service_inventory": dict(service_map),
        "action_inventory": dict(action_map),
        "launch_nodes": launch_nodes,
        "run_configs": parse_run_configs(),
    }
    OUT.write_text(json.dumps(out, indent=2), encoding="utf-8")
    print(f"Wrote {OUT}")


if __name__ == "__main__":
    main()
