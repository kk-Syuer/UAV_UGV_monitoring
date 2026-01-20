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
        '_last_hop_id',
        '_flow_type',
        '_control_type',
        '_seq',
        '_hop_count',
        '_ttl',
        '_requires_ack',
        '_payload',
        '_creation_time',
        '_ref_msg_id',
        '_last_tx_time',
        '_last_rx_time',
        '_drop_reason',
        '_recent_hops',
    ]

    _fields_and_field_types = {
        'msg_id': 'string',
        'src_id': 'string',
        'dst_id': 'string',
        'next_hop_id': 'string',
        'last_hop_id': 'string',
        'flow_type': 'uint8',
        'control_type': 'string',
        'seq': 'uint32',
        'hop_count': 'uint32',
        'ttl': 'uint32',
        'requires_ack': 'boolean',
        'payload': 'string',
        'creation_time': 'builtin_interfaces/Time',
        'ref_msg_id': 'string',
        'last_tx_time': 'builtin_interfaces/Time',
        'last_rx_time': 'builtin_interfaces/Time',
        'drop_reason': 'string',
        'recent_hops': 'sequence<string>',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint32'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['builtin_interfaces', 'msg'], 'Time'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['builtin_interfaces', 'msg'], 'Time'),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['builtin_interfaces', 'msg'], 'Time'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.UnboundedString()),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.msg_id = kwargs.get('msg_id', str())
        self.src_id = kwargs.get('src_id', str())
        self.dst_id = kwargs.get('dst_id', str())
        self.next_hop_id = kwargs.get('next_hop_id', str())
        self.last_hop_id = kwargs.get('last_hop_id', str())
        self.flow_type = kwargs.get('flow_type', int())
        self.control_type = kwargs.get('control_type', str())
        self.seq = kwargs.get('seq', int())
        self.hop_count = kwargs.get('hop_count', int())
        self.ttl = kwargs.get('ttl', int())
        self.requires_ack = kwargs.get('requires_ack', bool())
        self.payload = kwargs.get('payload', str())
        from builtin_interfaces.msg import Time
        self.creation_time = kwargs.get('creation_time', Time())
        self.ref_msg_id = kwargs.get('ref_msg_id', str())
        from builtin_interfaces.msg import Time
        self.last_tx_time = kwargs.get('last_tx_time', Time())
        from builtin_interfaces.msg import Time
        self.last_rx_time = kwargs.get('last_rx_time', Time())
        self.drop_reason = kwargs.get('drop_reason', str())
        self.recent_hops = kwargs.get('recent_hops', [])

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
        if self.last_hop_id != other.last_hop_id:
            return False
        if self.flow_type != other.flow_type:
            return False
        if self.control_type != other.control_type:
            return False
        if self.seq != other.seq:
            return False
        if self.hop_count != other.hop_count:
            return False
        if self.ttl != other.ttl:
            return False
        if self.requires_ack != other.requires_ack:
            return False
        if self.payload != other.payload:
            return False
        if self.creation_time != other.creation_time:
            return False
        if self.ref_msg_id != other.ref_msg_id:
            return False
        if self.last_tx_time != other.last_tx_time:
            return False
        if self.last_rx_time != other.last_rx_time:
            return False
        if self.drop_reason != other.drop_reason:
            return False
        if self.recent_hops != other.recent_hops:
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
    def last_hop_id(self):
        """Message field 'last_hop_id'."""
        return self._last_hop_id

    @last_hop_id.setter
    def last_hop_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'last_hop_id' field must be of type 'str'"
        self._last_hop_id = value

    @builtins.property
    def flow_type(self):
        """Message field 'flow_type'."""
        return self._flow_type

    @flow_type.setter
    def flow_type(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'flow_type' field must be of type 'int'"
            assert value >= 0 and value < 256, \
                "The 'flow_type' field must be an unsigned integer in [0, 255]"
        self._flow_type = value

    @builtins.property
    def control_type(self):
        """Message field 'control_type'."""
        return self._control_type

    @control_type.setter
    def control_type(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'control_type' field must be of type 'str'"
        self._control_type = value

    @builtins.property
    def seq(self):
        """Message field 'seq'."""
        return self._seq

    @seq.setter
    def seq(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'seq' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'seq' field must be an unsigned integer in [0, 4294967295]"
        self._seq = value

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

    @builtins.property
    def ttl(self):
        """Message field 'ttl'."""
        return self._ttl

    @ttl.setter
    def ttl(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'ttl' field must be of type 'int'"
            assert value >= 0 and value < 4294967296, \
                "The 'ttl' field must be an unsigned integer in [0, 4294967295]"
        self._ttl = value

    @builtins.property
    def requires_ack(self):
        """Message field 'requires_ack'."""
        return self._requires_ack

    @requires_ack.setter
    def requires_ack(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'requires_ack' field must be of type 'bool'"
        self._requires_ack = value

    @builtins.property
    def payload(self):
        """Message field 'payload'."""
        return self._payload

    @payload.setter
    def payload(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'payload' field must be of type 'str'"
        self._payload = value

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
    def ref_msg_id(self):
        """Message field 'ref_msg_id'."""
        return self._ref_msg_id

    @ref_msg_id.setter
    def ref_msg_id(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'ref_msg_id' field must be of type 'str'"
        self._ref_msg_id = value

    @builtins.property
    def last_tx_time(self):
        """Message field 'last_tx_time'."""
        return self._last_tx_time

    @last_tx_time.setter
    def last_tx_time(self, value):
        if __debug__:
            from builtin_interfaces.msg import Time
            assert \
                isinstance(value, Time), \
                "The 'last_tx_time' field must be a sub message of type 'Time'"
        self._last_tx_time = value

    @builtins.property
    def last_rx_time(self):
        """Message field 'last_rx_time'."""
        return self._last_rx_time

    @last_rx_time.setter
    def last_rx_time(self, value):
        if __debug__:
            from builtin_interfaces.msg import Time
            assert \
                isinstance(value, Time), \
                "The 'last_rx_time' field must be a sub message of type 'Time'"
        self._last_rx_time = value

    @builtins.property
    def drop_reason(self):
        """Message field 'drop_reason'."""
        return self._drop_reason

    @drop_reason.setter
    def drop_reason(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'drop_reason' field must be of type 'str'"
        self._drop_reason = value

    @builtins.property
    def recent_hops(self):
        """Message field 'recent_hops'."""
        return self._recent_hops

    @recent_hops.setter
    def recent_hops(self, value):
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 all(isinstance(v, str) for v in value) and
                 True), \
                "The 'recent_hops' field must be a set or sequence and each value of type 'str'"
        self._recent_hops = value
