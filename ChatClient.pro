QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = ChatClient
TEMPLATE = app

SOURCES += main.cpp \
           chatclient.cpp

HEADERS += chatclient.h