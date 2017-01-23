#!/usr/bin/env python
import os
import signal
from supervisor import childutils

def main():
    while True:
        headers, payload = childutils.listener.wait()
        if headers['eventname'] == 'PROCESS_STATE_EXITED':
            pheaders, pdata = childutils.eventdata(payload + '\n')
            if pheaders['processname'] == 'tests':
                os.kill(os.getppid(), signal.SIGTERM)
        childutils.listener.ok()

if __name__ == '__main__':
    main()
