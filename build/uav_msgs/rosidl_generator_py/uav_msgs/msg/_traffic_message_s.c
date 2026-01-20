// generated from rosidl_generator_py/resource/_idl_support.c.em
// with input from uav_msgs:msg/TrafficMessage.idl
// generated code does not contain a copyright notice
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <stdbool.h>
#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "numpy/ndarrayobject.h"
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif
#include "rosidl_runtime_c/visibility_control.h"
#include "uav_msgs/msg/detail/traffic_message__struct.h"
#include "uav_msgs/msg/detail/traffic_message__functions.h"

#include "rosidl_runtime_c/string.h"
#include "rosidl_runtime_c/string_functions.h"

#include "rosidl_runtime_c/primitives_sequence.h"
#include "rosidl_runtime_c/primitives_sequence_functions.h"

ROSIDL_GENERATOR_C_IMPORT
bool builtin_interfaces__msg__time__convert_from_py(PyObject * _pymsg, void * _ros_message);
ROSIDL_GENERATOR_C_IMPORT
PyObject * builtin_interfaces__msg__time__convert_to_py(void * raw_ros_message);
ROSIDL_GENERATOR_C_IMPORT
bool builtin_interfaces__msg__time__convert_from_py(PyObject * _pymsg, void * _ros_message);
ROSIDL_GENERATOR_C_IMPORT
PyObject * builtin_interfaces__msg__time__convert_to_py(void * raw_ros_message);
ROSIDL_GENERATOR_C_IMPORT
bool builtin_interfaces__msg__time__convert_from_py(PyObject * _pymsg, void * _ros_message);
ROSIDL_GENERATOR_C_IMPORT
PyObject * builtin_interfaces__msg__time__convert_to_py(void * raw_ros_message);

ROSIDL_GENERATOR_C_EXPORT
bool uav_msgs__msg__traffic_message__convert_from_py(PyObject * _pymsg, void * _ros_message)
{
  // check that the passed message is of the expected Python class
  {
    char full_classname_dest[45];
    {
      char * class_name = NULL;
      char * module_name = NULL;
      {
        PyObject * class_attr = PyObject_GetAttrString(_pymsg, "__class__");
        if (class_attr) {
          PyObject * name_attr = PyObject_GetAttrString(class_attr, "__name__");
          if (name_attr) {
            class_name = (char *)PyUnicode_1BYTE_DATA(name_attr);
            Py_DECREF(name_attr);
          }
          PyObject * module_attr = PyObject_GetAttrString(class_attr, "__module__");
          if (module_attr) {
            module_name = (char *)PyUnicode_1BYTE_DATA(module_attr);
            Py_DECREF(module_attr);
          }
          Py_DECREF(class_attr);
        }
      }
      if (!class_name || !module_name) {
        return false;
      }
      snprintf(full_classname_dest, sizeof(full_classname_dest), "%s.%s", module_name, class_name);
    }
    assert(strncmp("uav_msgs.msg._traffic_message.TrafficMessage", full_classname_dest, 44) == 0);
  }
  uav_msgs__msg__TrafficMessage * ros_message = _ros_message;
  {  // msg_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "msg_id");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->msg_id, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // src_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "src_id");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->src_id, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // dst_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "dst_id");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->dst_id, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // next_hop_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "next_hop_id");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->next_hop_id, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // last_hop_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "last_hop_id");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->last_hop_id, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // flow_type
    PyObject * field = PyObject_GetAttrString(_pymsg, "flow_type");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->flow_type = (uint8_t)PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // control_type
    PyObject * field = PyObject_GetAttrString(_pymsg, "control_type");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->control_type, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // seq
    PyObject * field = PyObject_GetAttrString(_pymsg, "seq");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->seq = PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // hop_count
    PyObject * field = PyObject_GetAttrString(_pymsg, "hop_count");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->hop_count = PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // ttl
    PyObject * field = PyObject_GetAttrString(_pymsg, "ttl");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->ttl = PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // requires_ack
    PyObject * field = PyObject_GetAttrString(_pymsg, "requires_ack");
    if (!field) {
      return false;
    }
    assert(PyBool_Check(field));
    ros_message->requires_ack = (Py_True == field);
    Py_DECREF(field);
  }
  {  // payload
    PyObject * field = PyObject_GetAttrString(_pymsg, "payload");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->payload, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // creation_time
    PyObject * field = PyObject_GetAttrString(_pymsg, "creation_time");
    if (!field) {
      return false;
    }
    if (!builtin_interfaces__msg__time__convert_from_py(field, &ros_message->creation_time)) {
      Py_DECREF(field);
      return false;
    }
    Py_DECREF(field);
  }
  {  // ref_msg_id
    PyObject * field = PyObject_GetAttrString(_pymsg, "ref_msg_id");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->ref_msg_id, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // last_tx_time
    PyObject * field = PyObject_GetAttrString(_pymsg, "last_tx_time");
    if (!field) {
      return false;
    }
    if (!builtin_interfaces__msg__time__convert_from_py(field, &ros_message->last_tx_time)) {
      Py_DECREF(field);
      return false;
    }
    Py_DECREF(field);
  }
  {  // last_rx_time
    PyObject * field = PyObject_GetAttrString(_pymsg, "last_rx_time");
    if (!field) {
      return false;
    }
    if (!builtin_interfaces__msg__time__convert_from_py(field, &ros_message->last_rx_time)) {
      Py_DECREF(field);
      return false;
    }
    Py_DECREF(field);
  }
  {  // drop_reason
    PyObject * field = PyObject_GetAttrString(_pymsg, "drop_reason");
    if (!field) {
      return false;
    }
    assert(PyUnicode_Check(field));
    PyObject * encoded_field = PyUnicode_AsUTF8String(field);
    if (!encoded_field) {
      Py_DECREF(field);
      return false;
    }
    rosidl_runtime_c__String__assign(&ros_message->drop_reason, PyBytes_AS_STRING(encoded_field));
    Py_DECREF(encoded_field);
    Py_DECREF(field);
  }
  {  // recent_hops
    PyObject * field = PyObject_GetAttrString(_pymsg, "recent_hops");
    if (!field) {
      return false;
    }
    {
      PyObject * seq_field = PySequence_Fast(field, "expected a sequence in 'recent_hops'");
      if (!seq_field) {
        Py_DECREF(field);
        return false;
      }
      Py_ssize_t size = PySequence_Size(field);
      if (-1 == size) {
        Py_DECREF(seq_field);
        Py_DECREF(field);
        return false;
      }
      if (!rosidl_runtime_c__String__Sequence__init(&(ros_message->recent_hops), size)) {
        PyErr_SetString(PyExc_RuntimeError, "unable to create String__Sequence ros_message");
        Py_DECREF(seq_field);
        Py_DECREF(field);
        return false;
      }
      rosidl_runtime_c__String * dest = ros_message->recent_hops.data;
      for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject * item = PySequence_Fast_GET_ITEM(seq_field, i);
        if (!item) {
          Py_DECREF(seq_field);
          Py_DECREF(field);
          return false;
        }
        assert(PyUnicode_Check(item));
        PyObject * encoded_item = PyUnicode_AsUTF8String(item);
        if (!encoded_item) {
          Py_DECREF(seq_field);
          Py_DECREF(field);
          return false;
        }
        rosidl_runtime_c__String__assign(&dest[i], PyBytes_AS_STRING(encoded_item));
        Py_DECREF(encoded_item);
      }
      Py_DECREF(seq_field);
    }
    Py_DECREF(field);
  }

  return true;
}

ROSIDL_GENERATOR_C_EXPORT
PyObject * uav_msgs__msg__traffic_message__convert_to_py(void * raw_ros_message)
{
  /* NOTE(esteve): Call constructor of TrafficMessage */
  PyObject * _pymessage = NULL;
  {
    PyObject * pymessage_module = PyImport_ImportModule("uav_msgs.msg._traffic_message");
    assert(pymessage_module);
    PyObject * pymessage_class = PyObject_GetAttrString(pymessage_module, "TrafficMessage");
    assert(pymessage_class);
    Py_DECREF(pymessage_module);
    _pymessage = PyObject_CallObject(pymessage_class, NULL);
    Py_DECREF(pymessage_class);
    if (!_pymessage) {
      return NULL;
    }
  }
  uav_msgs__msg__TrafficMessage * ros_message = (uav_msgs__msg__TrafficMessage *)raw_ros_message;
  {  // msg_id
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->msg_id.data,
      strlen(ros_message->msg_id.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "msg_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // src_id
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->src_id.data,
      strlen(ros_message->src_id.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "src_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // dst_id
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->dst_id.data,
      strlen(ros_message->dst_id.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "dst_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // next_hop_id
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->next_hop_id.data,
      strlen(ros_message->next_hop_id.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "next_hop_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // last_hop_id
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->last_hop_id.data,
      strlen(ros_message->last_hop_id.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "last_hop_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // flow_type
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->flow_type);
    {
      int rc = PyObject_SetAttrString(_pymessage, "flow_type", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // control_type
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->control_type.data,
      strlen(ros_message->control_type.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "control_type", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // seq
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->seq);
    {
      int rc = PyObject_SetAttrString(_pymessage, "seq", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // hop_count
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->hop_count);
    {
      int rc = PyObject_SetAttrString(_pymessage, "hop_count", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // ttl
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->ttl);
    {
      int rc = PyObject_SetAttrString(_pymessage, "ttl", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // requires_ack
    PyObject * field = NULL;
    field = PyBool_FromLong(ros_message->requires_ack ? 1 : 0);
    {
      int rc = PyObject_SetAttrString(_pymessage, "requires_ack", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // payload
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->payload.data,
      strlen(ros_message->payload.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "payload", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // creation_time
    PyObject * field = NULL;
    field = builtin_interfaces__msg__time__convert_to_py(&ros_message->creation_time);
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "creation_time", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // ref_msg_id
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->ref_msg_id.data,
      strlen(ros_message->ref_msg_id.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "ref_msg_id", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // last_tx_time
    PyObject * field = NULL;
    field = builtin_interfaces__msg__time__convert_to_py(&ros_message->last_tx_time);
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "last_tx_time", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // last_rx_time
    PyObject * field = NULL;
    field = builtin_interfaces__msg__time__convert_to_py(&ros_message->last_rx_time);
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "last_rx_time", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // drop_reason
    PyObject * field = NULL;
    field = PyUnicode_DecodeUTF8(
      ros_message->drop_reason.data,
      strlen(ros_message->drop_reason.data),
      "replace");
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "drop_reason", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // recent_hops
    PyObject * field = NULL;
    size_t size = ros_message->recent_hops.size;
    rosidl_runtime_c__String * src = ros_message->recent_hops.data;
    field = PyList_New(size);
    if (!field) {
      return NULL;
    }
    for (size_t i = 0; i < size; ++i) {
      PyObject * decoded_item = PyUnicode_DecodeUTF8(src[i].data, strlen(src[i].data), "replace");
      if (!decoded_item) {
        return NULL;
      }
      int rc = PyList_SetItem(field, i, decoded_item);
      (void)rc;
      assert(rc == 0);
    }
    assert(PySequence_Check(field));
    {
      int rc = PyObject_SetAttrString(_pymessage, "recent_hops", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }

  // ownership of _pymessage is transferred to the caller
  return _pymessage;
}
