# [qiaqia](https://github.com/Andrewpqc/qiaqia)
A command line chat application.
```
 _____     _       ___   _____     _       ___             　　　　　　　　　　　　
/  _  |   | |     /   | /  _  |   | |     /   |            
| | | |   | |    / /| | | | | |   | |    / /| |    Welcome to QIAQIA(洽洽)
| | | |   | |   / / | | | | | |   | |   / / | |   
| |_| |_  | |  / /  | | | |_| |_  | |  / /  | |
|_______| |_| /_/   |_| |_______| |_| /_/   |_|
```
## features
+ event based IO multiplexing concurrency model for qiaqia server(epoll).
+ support group chat and private chat.
+ You can choose to receive or not receive messages from specific online users(block or unblock).
+ support state query such as infomations of all online users and the users you blocked.

## usage
```
    <message>               : send message to all online users that not block you.
    $ <commands>            : send a command to the qiaqia for state query.
    > <username> <message>  : send a message to a single user that not block you.
    # <username>            : block or unblock the messages from certain user 
```

## example
```
    exit                    : disconnect to the server and leave.
    clear                   : clear the screen, just like the clear in bash.
    hello, guys!            : send 'hello, guys!' to all online users that not block you.
    $ show users            : show informations of all currently online users
    $ show blocked          : show the users that you blocked
    > Bob how are you?      : send 'how are you?' only to Bob if bob not block you.
    # Bob                   : block messages from Bob.
    # !Bob                  : unblock messages from Bob.
    # Mike !Amy ...         : block Mick and unblock Amy.
```
## install
Qiaqia is made up of two components, the `qiaqia_server` and the `qiaqia_client`. So you have three choices to install:
+ install `qiaqia_server`
``` bash
git clone https://github.com/Andrewpqc/qiaqia.git && cd qiaqia && make server && sudo make install
```
+ install `qiaqia_client`
``` bash
git clone https://github.com/Andrewpqc/qiaqia.git && cd qiaqia && make client && sudo make install
```

+ both
``` bash
git clone https://github.com/Andrewpqc/qiaqia.git && cd qiaqia && make && sudo make install
```
Once you have finished one of the commands, the corresponding executable binary file will be placed under the `/usr/local/bin/` directory of your machine. If you want to uninstall Qiaqia,just to type: 
``` bash
sudo rm /usr/local/bin/qiaqia*
```
in you terminal.


Before you start `qiaqia_client`, you must make sure that `qiaqia_server` is already started on your local machine or anywhere network can reach.

You can start `qiaqia_server` like the following command:
``` bash
qiaqia_server -p <port to be listen>
```
I already deployed a `qiaqia_server` on my machine at 120.77.220.239:8765, so you can quick start like the following:
``` bash
qiaqia_client -h 120.77.220.239 -p 8765
```
then,choose a pretty nickname for yourself and start. Enjory!
