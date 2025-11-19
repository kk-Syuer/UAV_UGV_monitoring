from setuptools import find_packages
from setuptools import setup

setup(
    name='uav_msgs',
    version='0.0.0',
    packages=find_packages(
        include=('uav_msgs', 'uav_msgs.*')),
)
