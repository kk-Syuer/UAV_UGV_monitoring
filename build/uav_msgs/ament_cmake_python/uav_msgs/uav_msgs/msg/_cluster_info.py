# generated from rosidl_generator_py/resource/_idl.py.em
# with input from uav_msgs:msg/ClusterInfo.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_ClusterInfo(type):
    """Metaclass of message 'ClusterInfo'."""

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
                'uav_msgs.msg.ClusterInfo')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__cluster_info
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__cluster_info
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__cluster_info
            cls._TYPE_SUPPORT = module.type_support_msg__msg__cluster_info
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__cluster_info

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class ClusterInfo(metaclass=Metaclass_ClusterInfo):
    """Message class 'ClusterInfo'."""

    __slots__ = [
        '_cluster_id',
        '_ch_id',
        '_member_ids',
    ]

    _fields_and_field_types = {
        'cluster_id': 'string',
        'ch_id': 'string',
        'member_ids': 'sequence<string>',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedSequence(rosidl_parser.definition.UnboundedString()),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.cluster_id = kwargs.get('cluster_id', str())
        self.ch_id = kwargs.get('ch_id', str())
        self.member_ids = kwargs.get('member_ids', [])

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
        if self.cluster_id != other.cluster_id:
            return False
        if self.ch_id != other.ch_id:
            return False
        if self.member_ids != other.member_ids:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

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
    def member_ids(self):
        """Message field 'member_ids'."""
        return self._member_ids

    @member_ids.setter
    def member_ids(self, value):
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
                "The 'member_ids' field must be a set or sequence and each value of type 'str'"
        self._member_ids = value
