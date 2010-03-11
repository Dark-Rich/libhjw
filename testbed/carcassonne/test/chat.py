
import sys, os, re
sys.path.insert(0, os.path.split(os.path.abspath('.'))[0])

from twisted.python import log

from twisted.web.websocket import WebSocketSite
from twisted.web.resource import Resource
from twisted.web import static
from wsjson import *


class ChatHandler(WSJsonRPCHandler):

    all_users = {}                          # username -> handler

    def broadcast(self, msg, exceptThis=False):
        for h in self.all_users.itervalues():
            if h == self and exceptThis:
                continue
            h.notify('msg', msg)

    def do_chat(self, msg):
        if not hasattr(self, 'username'):
            return False
        self.broadcast('%s said: %s' % (self.username, msg))
        return True

    USERNAME_re = re.compile(r"^[_A-z]\w{0,19}$").match
    def do_login(self, username):
        if hasattr(self, 'username'):
            return False

        if type(username) not in (unicode, str) or not self.USERNAME_re(username):
            return False

        if username in self.all_users:
            return False

        self.username = username
        self.all_users[username] = self
        self.broadcast('%s entered the chatroom' % username, exceptThis=True)
        return True

    def connectionLost(self, reason):
        if hasattr(self, 'username'):
            self.broadcast('%s exited the chatroom' % self.username, exceptThis=True)
            del self.all_users[self.username]


def main():
    from twisted.internet import reactor
    log.startLogging(sys.stdout)
    root = static.File('.')
    site = WebSocketSite(root)
    site.addHandler("/ws/chat", ChatHandler)
    reactor.listenTCP(9876, site)
    reactor.run()


if __name__ == "__main__":
    main() 