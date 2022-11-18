from flask import Flask

app = Flask(__name__)
# pylint: disable=C0413
from app import views
