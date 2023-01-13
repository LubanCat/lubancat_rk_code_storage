import sys
from PyQt5.QtWidgets import QApplication, QMainWindow
from login_ui  import Ui_Form


class LoginWindow(QMainWindow):

    def __init__(self, parent=None):
        super().__init__(parent)
        # 
        self.ui = Ui_Form()
        #
        self.ui.setupUi(self)
        # 
        self.setWindowTitle("Lubancat login")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = LoginWindow()
    win.show()
    sys.exit(app.exec())
