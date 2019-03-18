Nickolas Gough, nvg081, 11181823


Running process:

    make process
    ./process <pid> <data> <D> <port> <logger address> <logger port>

Example:

    make process
    ./process 0 "msg 0" 100 30001 tux8.usask.ca 30000


Running logger:

    make logger
    ./logger <port> <T> <N>

Example:

    make logger
    ./logger 30000 200 5

Notes:

    The logger will print the events of the simulation as they occur in
    real time.
