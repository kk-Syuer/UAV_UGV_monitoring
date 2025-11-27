from setuptools import setup

package_name = 'planner_viz'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
         ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='vboxuser',
    maintainer_email='liyu.jin03@gmail.com',
    description='Coverage planner visualizer',
    license='TODO',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'planner_viz_node = planner_viz.planner_viz_node:main',
        ],
    },
)
