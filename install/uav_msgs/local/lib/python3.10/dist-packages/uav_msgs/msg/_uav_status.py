# generated from rosidl_generator_py/resource/_idl.py.em
# with input from uav_msgs:msg/UavStatus.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import math  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_UavStatus(type):
    """Metaclass of message 'UavStatus'."""

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
                'uav_msgs.msg.UavStatus')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__uav_status
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__uav_status
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__uav_status
            cls._TYPE_SUPPORT = module.type_support_msg__msg__uav_status
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__uav_status

            from builtin_interfaces.msg import Time
            if Time.__class__._TYPE_SUPPORT is None:
                Time.__class__.__import_type_support__()

            from geometry_msgs.msg import Pose
            if Pose.__class__._TYPE_SUPPORT is None:
                Pose.__class__.__import_type_support__()

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class UavStatus(metaclass=Metaclass_UavStatus):
    """Message class 'UavStatus'."""

    __slots__ = [
        '_uav_id',
        '_role',
        '_cluster_id',
        '_battery_level',
        '_battery_capacity',
        '_pose',
        '_service_radius',
        '_connected_users',
        '_traffic_load',
        '_packet_loss_estimate',
        '_energy_consumption_rate',
        '_stamp',
    ]

    _fields_and_field_types = {
        'uav_id': 'string',
        'role': 'uint8',
        'cluster_id': 'string',
        'battery_level': 'float',
        'battery_capacity': 'float',
        'pose': 'geometry_msgs/Pose',
        'service_radius': 'float',
        'connected_users': 'uint32',
        'traffic_load': 'float',
        'packet_loss_estimate': 'float',
        'energy_consumption_rate': 'float',
        'stamp': 'builtin_interfaces/Time',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['geometry_msgs', 'msg'], 'Pose'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['builtin_interfaces', 'msg'], 'Time'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.uav_id = kwargs.get('uav_id', str())
        self.role = kwargs.get('role', int())
        self.cluster_id = kwargs.get('cluster_id', str())
        self.battery_level = kwargs.get('battery_level', float())
        self.battery_capacity = kwargs.get('battery_capacity', float())
        from geometry_msgs.msg import Pose
        self.pose = kwargs.get('pose', Pose())
        self.service_radius = kwargs.get('service_radius', float())
        self.connected_users = kwargs.get('connected_users', int())
        self.traffic_load = kwargs.get('traffic_load', float())
        self.packet_loss_estimate = kwargs.get('packet_loss_estimate', float())
        self.energy_consumption_rate = kwargs.get('energy_consumption_rate', float())
        from builtin_interfaces.msg import Time
        self.stamp = kwargs.get('stamp', Time())

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
        if self.uav_id != other.uav_id:
            return False
        if self.role != other.role:
            return False
        if self.cluster_id != other.cluster_id:
            return False
        if self.battery_level != other.battery_level:
            return False
        if self.battery_capacity != other.battery_capacity:
            return False
        if self.pose != other.pose:
            return False
        if self.service_radius != other.service_radius:
            return False
        if self.connected_users != other.connected_users:
            return False
        if self.traffic_load != other.traffic_load:
            return False
        if self.packet_loss_estimate != other.packet_loss_estimate:
            return False
        if self.energy_consumption_rate != other.energy_consumption_rate:
            return False
        if self.stamp != other.stamp:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def uav_id(self):
        """Message field 'uav_id'."""
        return self._uav_id

    @uav_id.setter
    def uav_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'uav_id' field must be of type 'str'"
        self._uav_id = value

    @builtins.property
    def role(self):
        """Message field 'role'."""
        return self._role

    @role.setter
    def role(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'role' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'role' field must be an unsigned integer in [0, 255]"
        self._role = value

    @builtins.property
    def cluster_id(self):
        """Message field 'cluster_id'."""
        return self._cluster_id

    @cluster_id.setter
    def cluster_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'cluster_id' field must be of type 'str'"
        self._cluster_id = value

    @builtins.property
    def battery_level(self):
        """Message field 'battery_level'."""
        return self._battery_level

    @battery_level.setter
    def battery_level(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'battery_level' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'battery_level' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._battery_level = value

    @builtins.property
    def battery_capacity(self):
        """Message field 'battery_capacity'."""
        return self._battery_capacity

    @battery_capacity.setter
    def battery_capacity(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'battery_capacity' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'battery_capacity' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._battery_capacity = value

    @builtins.property
    def pose(self):
        """Message field 'pose'."""
        return self._pose

    @pose.setter
    def pose(self, value):
        if __debug__:
            from geometry_msgs.msg import Pose
            assert \
                isinstance(value, Pose), \
                "The 'pose' field must be a sub message of type 'Pose'"
        self._pose = value

    @builtins.property
    def service_radius(self):
        """Message field 'service_radius'."""
        return self._service_radius

    @service_radius.setter
    def service_radius(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'service_radius' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'service_radius' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._service_radius = value

    @builtins.property
    def connected_users(self):
        """Message field 'connected_users'."""
        return self._connected_users

    @connected_users.setter
    def connected_users(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'connected_users' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'connected_users' field must be an unsigned integer in [0, 4294967295]"
        self._connected_users = value

    @builtins.property
    def traffic_load(self):
        """Message field 'traffic_load'."""
        return self._traffic_load

    @traffic_load.setter
    def traffic_load(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'traffic_load' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'traffic_load' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._traffic_load = value

    @builtins.property
    def packet_loss_estimate(self):
        """Message field 'packet_loss_estimate'."""
        return self._packet_loss_estimate

    @packet_loss_estimate.setter
    def packet_loss_estimate(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'packet_loss_estimate' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'packet_loss_estimate' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._packet_loss_estimate = value

    @builtins.property
    def energy_consumption_rate(self):
        """Message field 'energy_consumption_rate'."""
        return self._energy_consumption_rate

    @energy_consumption_rate.setter
    def energy_consumption_rate(self, value):
        if __debug__:
            assert \
                isinstance(value, float), \
                "The 'energy_consumption_rate' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'energy_consumption_rate' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._energy_consumption_rate = value

    @builtins.property
    def stamp(self):
        """Message field 'stamp'."""
        return self._stamp

    @stamp.setter
    def stamp(self, value):
        if __debug__:
            from builtin_interfaces.msg import Time
            assert \
                isinstance(value, Time), \
                "The 'stamp' field must be a sub message of type 'Time'"
        self._stamp = value
