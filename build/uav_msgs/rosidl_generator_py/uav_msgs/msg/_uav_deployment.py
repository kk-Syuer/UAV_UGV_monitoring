# generated from rosidl_generator_py/resource/_idl.py.em
# with input from uav_msgs:msg/UavDeployment.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_UavDeployment(type):
    """Metaclass of message 'UavDeployment'."""

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
                'uav_msgs.msg.UavDeployment')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__uav_deployment
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__uav_deployment
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__uav_deployment
            cls._TYPE_SUPPORT = module.type_support_msg__msg__uav_deployment
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__uav_deployment

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


class UavDeployment(metaclass=Metaclass_UavDeployment):
    """Message class 'UavDeployment'."""

    __slots__ = [
        '_uav_id',
        '_target_pose',
        '_role',
        '_cluster_id',
        '_ch_id',
        '_next_hop_to_sink',
    ]

    _fields_and_field_types = {
        'uav_id': 'string',
        'target_pose': 'geometry_msgs/Pose',
        'role': 'uint8',
        'cluster_id': 'string',
        'ch_id': 'string',
        'next_hop_to_sink': 'string',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['geometry_msgs', 'msg'], 'Pose'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.uav_id = kwargs.get('uav_id', str())
        from geometry_msgs.msg import Pose
        self.target_pose = kwargs.get('target_pose', Pose())
        self.role = kwargs.get('role', int())
        self.cluster_id = kwargs.get('cluster_id', str())
        self.ch_id = kwargs.get('ch_id', str())
        self.next_hop_to_sink = kwargs.get('next_hop_to_sink', str())

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
        if self.target_pose != other.target_pose:
            return False
        if self.role != other.role:
            return False
        if self.cluster_id != other.cluster_id:
            return False
        if self.ch_id != other.ch_id:
            return False
        if self.next_hop_to_sink != other.next_hop_to_sink:
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
    def target_pose(self):
        """Message field 'target_pose'."""
        return self._target_pose

    @target_pose.setter
    def target_pose(self, value):
        if __debug__:
            from geometry_msgs.msg import Pose
            assert \
                isinstance(value, Pose), \
                "The 'target_pose' field must be a sub message of type 'Pose'"
        self._target_pose = value

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
    def ch_id(self):
        """Message field 'ch_id'."""
        return self._ch_id

    @ch_id.setter
    def ch_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'ch_id' field must be of type 'str'"
        self._ch_id = value

    @builtins.property
    def next_hop_to_sink(self):
        """Message field 'next_hop_to_sink'."""
        return self._next_hop_to_sink

    @next_hop_to_sink.setter
    def next_hop_to_sink(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'next_hop_to_sink' field must be of type 'str'"
        self._next_hop_to_sink = value
