#!/usr/bin/env python3
# Copyright 2011 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.

import sys
import ngagecc
from tools import shared

if __name__ == '__main__':
  try:
    sys.exit(ngagecc.main(sys.argv))
  except KeyboardInterrupt:
    ngagecc.logger.debug('KeyboardInterrupt')
    sys.exit(1)
