#!/usr/bin/env bash
# Configure a vagrant system for building WWIIOL

. ubuntu_provision.sh

chown vagrant.vagrant /playnet /playnet/wwiiolng
