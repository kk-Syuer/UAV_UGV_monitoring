# generated from rosidl_generator_py/resource/_idl.py.em
# with input from uav_msgs:msg/ChargeDecision.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_ChargeDecision(type):
    """Metaclass of message 'ChargeDecision'."""

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
                'uav_msgs.msg.ChargeDecision')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__charge_decision
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__charge_decision
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__charge_decision
            cls._TYPE_SUPPORT = module.type_support_msg__msg__charge_decision
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__charge_decision

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


class ChargeDecision(metaclass=Metaclass_ChargeDecision):
    """Message class 'ChargeDecision'."""

    __slots__ = [
        '_uav_id',
        '_accepted',
        '_slot_start_time',
        '_priority',
        '_policy',
    ]

    _fields_and_field_types = {
        'uav_id': 'string',
        'accepted': 'boolean',
        'slot_start_time': 'builtin_interfaces/Time',
        'priority': 'uint8',
        'policy': 'string',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
        rosidl_parser.definition.NamespacedType(['builtin_interfaces', 'msg'], 'Time'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint8'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.uav_id = kwargs.get('uav_id', str())
        self.accepted = kwargs.get('accepted', bool())
        from builtin_interfaces.msg import Time
        self.slot_start_time = kwargs.get('slot_start_time', Time())
        self.priority = kwargs.get('priority', int())
        self.policy = kwargs.get('policy', str())

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
        if self.accepted != other.accepted:
            return False
        if self.slot_start_time != other.slot_start_time:
            return False
        if self.priority != other.priority:
            return False
        if self.policy != other.policy:
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
    def accepted(self):
        """Message field 'accepted'."""
        return self._accepted

    @accepted.setter
    def accepted(self, value):
        if __debug__:
            assert \
                isinstance(value, bool), \
                "The 'accepted' field must be of type 'bool'"
        self._accepted = value

    @builtins.property
    def slot_start_time(self):
        """Message field 'slot_start_time'."""
        return self._slot_start_time

    @slot_start_time.setter
    def slot_start_time(self, value):
        if __debug__:
            from builtin_interfaces.msg import Time
            assert \
                isinstance(value, Time), \
                "The 'slot_start_time' field must be a sub message of type 'Time'"
        self._slot_start_time = value

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
    def policy(self):
        """Message field 'policy'."""
        return self._policy

    @policy.setter
    def policy(self, value):
        if __debug__:
            assert \
                isinstance(value, str), \
                "The 'policy' field must be of type 'str'"
        self._policy = value
