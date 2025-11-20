# generated from rosidl_generator_py/resource/_idl.py.em
# with input from uav_msgs:msg/WeatherStatus.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import math  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_WeatherStatus(type):
    """Metaclass of message 'WeatherStatus'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('uav_msgs')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'uav_msgs.msg.WeatherStatus')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__weather_status
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__weather_status
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__weather_status
            cls._TYPE_SUPPORT = module.type_support_msg__msg__weather_status
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__weather_status

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class WeatherStatus(metaclass=Metaclass_WeatherStatus):
    """Message class 'WeatherStatus'."""

    __slots__ = [
        '_rain_intensity',
        '_wind_speed',
        '_wind_direction_deg',
        '_temperature_c',
    ]

    _fields_and_field_types = {
        'rain_intensity': 'float',
        'wind_speed': 'float',
        'wind_direction_deg': 'float',
        'temperature_c': 'float',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.rain_intensity = kwargs.get('rain_intensity', float())
        self.wind_speed = kwargs.get('wind_speed', float())
        self.wind_direction_deg = kwargs.get('wind_direction_deg', float())
        self.temperature_c = kwargs.get('temperature_c', float())

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.__slots__, self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s[1:] + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.rain_intensity != other.rain_intensity:
            return False
        if self.wind_speed != other.wind_speed:
            return False
        if self.wind_direction_deg != other.wind_direction_deg:
            return False
        if self.temperature_c != other.temperature_c:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def rain_intensity(self):
        """Message field 'rain_intensity'."""
        return self._rain_intensity

    @rain_intensity.setter
    def rain_intensity(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'rain_intensity' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'rain_intensity' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._rain_intensity = value

    @builtins.property
    def wind_speed(self):
        """Message field 'wind_speed'."""
        return self._wind_speed

    @wind_speed.setter
    def wind_speed(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'wind_speed' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'wind_speed' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._wind_speed = value

    @builtins.property
    def wind_direction_deg(self):
        """Message field 'wind_direction_deg'."""
        return self._wind_direction_deg

    @wind_direction_deg.setter
    def wind_direction_deg(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'wind_direction_deg' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'wind_direction_deg' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._wind_direction_deg = value

    @builtins.property
    def temperature_c(self):
        """Message field 'temperature_c'."""
        return self._temperature_c

    @temperature_c.setter
    def temperature_c(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'temperature_c' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'temperature_c' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._temperature_c = value
