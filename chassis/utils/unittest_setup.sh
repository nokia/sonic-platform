#!/bin/bash

usage()
{
cat << EOF
usage: ./unittest_setup.sh -i ipaddress -p port
-i    | --ip-address                 IP address of the GRPC-server
-p    | --port                       Port of the GRPC-server
-h    | --help                       Brings up this menu
EOF
}

while [ "$1" != "" ]; do
    case $1 in
        -i | --ip-address )
            shift
            ip_address=$1
        ;;
        -p | --port )
            shift
            port=$1
        ;;
        -h | --help )    usage
            exit
        ;;
        * )              usage
            exit 1
    esac
    shift
done

#Check for ip-address
if [[ $ip_address == "" || $port == "" ]]; then
    usage
    exit
fi

#Add the IP:Port to a file
echo "$ip_address:$port" > /tmp/nokia_grpc_server

#Install dependent packages
pip install protobuf grpcio
pip install /sonic/target/python-wheels/swsssdk-2.0.1-py2-none-any.whl
pip install /sonic/target/python-wheels/sonic_py_common-1.0-py2-none-any.whl
pip install /sonic/target/python-wheels/sonic_platform_common-1.0-py2-none-any.whl

echo "************************************************************************"
echo "Run unit-tests using pytest. For ex:"
echo "pytest platform_tests or"
echo "pytest platform_tests/test_psu.py"
echo "************************************************************************"
