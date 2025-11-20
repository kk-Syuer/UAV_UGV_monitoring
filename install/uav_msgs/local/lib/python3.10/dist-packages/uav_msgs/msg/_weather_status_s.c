// generated from rosidl_generator_py/resource/_idl_support.c.em
// with input from uav_msgs:msg/WeatherStatus.idl
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
#include "uav_msgs/msg/detail/weather_status__struct.h"
#include "uav_msgs/msg/detail/weather_status__functions.h"


ROSIDL_GENERATOR_C_EXPORT
bool uav_msgs__msg__weather_status__convert_from_py(PyObject * _pymsg, void * _ros_message)
{
  // check that the passed message is of the expected Python class
  {
    char full_classname_dest[43];
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
    assert(strncmp("uav_msgs.msg._weather_status.WeatherStatus", full_classname_dest, 42) == 0);
  }
  uav_msgs__msg__WeatherStatus * ros_message = _ros_message;
  {  // rain_intensity
    PyObject * field = PyObject_GetAttrString(_pymsg, "rain_intensity");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->rain_intensity = (float)PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // wind_speed
    PyObject * field = PyObject_GetAttrString(_pymsg, "wind_speed");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->wind_speed = (float)PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // wind_direction_deg
    PyObject * field = PyObject_GetAttrString(_pymsg, "wind_direction_deg");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->wind_direction_deg = (float)PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }
  {  // temperature_c
    PyObject * field = PyObject_GetAttrString(_pymsg, "temperature_c");
    if (!field) {
      return false;
    }
    assert(PyFloat_Check(field));
    ros_message->temperature_c = (float)PyFloat_AS_DOUBLE(field);
    Py_DECREF(field);
  }

  return true;
}

ROSIDL_GENERATOR_C_EXPORT
PyObject * uav_msgs__msg__weather_status__convert_to_py(void * raw_ros_message)
{
  /* NOTE(esteve): Call constructor of WeatherStatus */
  PyObject * _pymessage = NULL;
  {
    PyObject * pymessage_module = PyImport_ImportModule("uav_msgs.msg._weather_status");
    assert(pymessage_module);
    PyObject * pymessage_class = PyObject_GetAttrString(pymessage_module, "WeatherStatus");
    assert(pymessage_class);
    Py_DECREF(pymessage_module);
    _pymessage = PyObject_CallObject(pymessage_class, NULL);
    Py_DECREF(pymessage_class);
    if (!_pymessage) {
      return NULL;
    }
  }
  uav_msgs__msg__WeatherStatus * ros_message = (uav_msgs__msg__WeatherStatus *)raw_ros_message;
  {  // rain_intensity
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->rain_intensity);
    {
      int rc = PyObject_SetAttrString(_pymessage, "rain_intensity", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // wind_speed
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->wind_speed);
    {
      int rc = PyObject_SetAttrString(_pymessage, "wind_speed", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // wind_direction_deg
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->wind_direction_deg);
    {
      int rc = PyObject_SetAttrString(_pymessage, "wind_direction_deg", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // temperature_c
    PyObject * field = NULL;
    field = PyFloat_FromDouble(ros_message->temperature_c);
    {
      int rc = PyObject_SetAttrString(_pymessage, "temperature_c", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }

  // ownership of _pymessage is transferred to the caller
  return _pymessage;
}
