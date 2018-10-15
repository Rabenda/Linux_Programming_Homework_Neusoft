TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CFLAGS += -pthread
LIBS += -pthread

SOURCES += \
        main.c \
        server.c

HEADERS += \
    config.h \
    server.h


