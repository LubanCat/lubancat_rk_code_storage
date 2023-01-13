import sys
from PyQt5.QtWidgets import QApplication, QMainWindow
from PyQt5 import uic

class LoginWindow(QMainWindow):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        uic.loadUi("login.ui", self)
        self.setWindowTitle("Lubancat login")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = LoginWindow()
    window.show()
    sys.exit(app.exec_())


