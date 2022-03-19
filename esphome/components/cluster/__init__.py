import re
import time
import random

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NAME
from esphome.core import CORE

# TODO: need to be a platform component?
# IS_PLATFORM_COMPONENT = True

DEPENDENCIES = ['network']
AUTO_LOAD = ['json', 'async_udp']

CONF_CLUSTER_NAME = 'cluster_name'
CONF_PORT = 'port'
CONF_TIMEOUT = 'timeout'
CONF_WEIGHT = 'weight'
CONF_QUORUM = 'quorum'
CONF_AUTO_DISCOVERY = 'auto_discovery'
CONF_PEER_ADDRESSES = 'peers'

def ipv4_with_port(value):
    value = cv.string_strict(value)
    pattern = re.compile(r'^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3}):(\d{1,5})$')
    match = pattern.match(value)

    if match is None:
        raise cv.Invalid("IPv4 peer address must be in the format <ip-address>:<port>")

    parts = list(match.groups())
    port = int(parts.pop(-1))
    if not 0 < port <= 65535:
        raise cv.Invalid("Port must greater than 0 and less than or equal to 65535")

    parts_ = list(map(int, parts))
    if not all(0 <= x < 256 for x in parts_):
        raise cv.Invalid("IPv4 address parts must be in range from 0 to 255")

    return value

cluster_ns = cg.esphome_ns.namespace('cluster')
Cluster = cluster_ns.class_('Cluster', cg.Component, cg.Controller)

def validate(config):
    if CONF_NAME not in config:
        config[CONF_NAME] = CORE.name
    return config

CONFIG_SCHEMA = cv.All(cv.Schema({
    cv.GenerateID(): cv.declare_id(Cluster),
    cv.Optional(CONF_NAME): cv.valid_name,
    cv.Optional(CONF_CLUSTER_NAME, default='esphome'): cv.valid_name,
    cv.Optional(CONF_PORT, default=6425): cv.int_,
    cv.Optional(CONF_TIMEOUT, default=60): cv.int_,
    cv.Optional(CONF_WEIGHT, default=round(random.random() * time.time())): cv.int_,
    cv.Optional(CONF_QUORUM, default=0): cv.int_,
    cv.Optional(CONF_AUTO_DISCOVERY, default=True): cv.boolean,
    cv.Optional(CONF_PEER_ADDRESSES, default=[]): cv.Schema([ipv4_with_port]),
}).extend(cv.COMPONENT_SCHEMA), validate)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_cluster_name(config[CONF_CLUSTER_NAME]))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_timeout(config[CONF_TIMEOUT]))
    cg.add(var.set_weight(config[CONF_WEIGHT]))
    cg.add(var.set_quorum(config[CONF_QUORUM]))
    cg.add(var.set_auto_discovery(config[CONF_AUTO_DISCOVERY]))

    cg.add_define('USE_CLUSTER')
