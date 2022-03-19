# Dummy integration to allow relying on AsyncUDP
import esphome.codegen as cg
from esphome.core import CORE, coroutine_with_priority

@coroutine_with_priority(200.0)
def to_code(config):
    if CORE.is_esp8266:
        cg.add_library('ESPAsyncUDP', '1.1.0')
