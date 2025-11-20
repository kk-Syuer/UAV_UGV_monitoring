# generated from rosidl_generator_py/resource/_idl.py.em
# with input from uav_msgs:msg/TrafficMessage.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_TrafficMessage(type):
    """Metaclass of message 'TrafficMessage'."""

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
                'uav_msgs.msg.TrafficMessage')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__traffic_message
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__traffic_message
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__traffic_message
            cls._TYPE_SUPPORT = module.type_support_msg__msg__traffic_message
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__traffic_message

            from builtin_interfaces.msg import Time
            if Time.__class__._TYPE_SUPPORT is None:
                Time.__class__.__import_type_support__()

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class TrafficMessage(metaclass=Metaclass_TrafficMessage):
    """Message class 'TrafficMessage'."""

    __slots__ = [
        '_msg_id',
        '_src_id',
        '_dst_id',
        '_next_hop_id',
        '_msg_type',
        '_priority',
        '_size_bytes',
        '_creation_time',
        '_hop_count',
    ]

    _fields_and_field_types = {
        'msg_id': 'string',
        'src_id': 'string',
        'dst_id': 'string',
        'next_hop_id': 'string',
        'msg_type': 'uint8',
        'priority': 'uint8',
        'size_bytes': 'uint32',
        'creation_time': 'builtin_interfaces/Time',
        'hop_count': 'uint32',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['builtin_interfaces', 'msg'], 'Time'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.msg_id = kwargs.get('msg_id', str())
        self.src_id = kwargs.get('src_id', str())
        self.dst_id = kwargs.get('dst_id', str())
        self.next_hop_id = kwargs.get('next_hop_id', str())
        self.msg_type = kwargs.get('msg_type', int())
        self.priority = kwargs.get('priority', int())
        self.size_bytes = kwargs.get('size_bytes', int())
        from builtin_interfaces.msg import Time
        self.creation_time = kwargs.get('creation_time', Time())
        self.hop_count = kwargs.get('hop_count', int())

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
        if self.msg_id != other.msg_id:
            return False
        if self.src_id != other.src_id:
            return False
        if self.dst_id != other.dst_id:
            return False
        if self.next_hop_id != other.next_hop_id:
            return False
        if self.msg_type != other.msg_type:
            return False
        if self.priority != other.priority:
            return False
        if self.size_bytes != other.size_bytes:
            return False
        if self.creation_time != other.creation_time:
            return False
        if self.hop_count != other.hop_count:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def msg_id(self):
        """Message field 'msg_id'."""
        return self._msg_id

    @msg_id.setter
    def msg_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'msg_id' field must be of type 'str'"
        self._msg_id = value

    @builtins.property
    def src_id(self):
        """Message field 'src_id'."""
        return self._src_id

    @src_id.setter
    def src_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'src_id' field must be of type 'str'"
        self._src_id = value

    @builtins.property
    def dst_id(self):
        """Message field 'dst_id'."""
        return self._dst_id

    @dst_id.setter
    def dst_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'dst_id' field must be of type 'str'"
        self._dst_id = value

    @builtins.property
    def next_hop_id(self):
        """Message field 'next_hop_id'."""
        return self._next_hop_id

    @next_hop_id.setter
    def next_hop_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'next_hop_id' field must be of type 'str'"
        self._next_hop_id = value

    @builtins.property
    def msg_type(self):
        """Message field 'msg_type'."""
        return self._msg_type

    @msg_type.setter
    def msg_type(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'msg_type' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'msg_type' field must be an unsigned integer in [0, 255]"
        self._msg_type = value

    @builtins.property
    def priority(self):
        """Message field 'priority'."""
        return self._priority

    @priority.setter
    def priority(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'priority' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'priority' field must be an unsigned integer in [0, 255]"
        self._priority = value

    @builtins.property
    def size_bytes(self):
        """Message field 'size_bytes'."""
        return self._size_bytes

    @size_bytes.setter
    def size_bytes(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'size_bytes' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'size_bytes' field must be an unsigned integer in [0, 4294967295]"
        self._size_bytes = value

    @builtins.property
    def creation_time(self):
        """Message field 'creation_time'."""
        return self._creation_time

    @creation_time.setter
    def creation_time(self, value):
        if __debug__:
            from builtin_interfaces.msg import Time
            assert \
                isinstance(value, Time), \
                "The 'creation_time' field must be a sub message of type 'Time'"
        self._creation_time = value

    @builtins.property
    def hop_count(self):
        """Message field 'hop_count'."""
        return self._hop_count

    @hop_count.setter
    def hop_count(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'hop_count' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'hop_count' field must be an unsigned integer in [0, 4294967295]"
        self._hop_count = value
