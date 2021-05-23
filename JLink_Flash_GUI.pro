QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    jlink_settings.cpp \
    main.cpp \
    mainwindow.cpp \
    src/hj_file_write.cpp \
    utils/utils.cpp

HEADERS += \
    hj_file_write.h \
    jlink_settings.h \
    mainwindow.h \
    src/hj_file_write.h \
    utils/utils.h

FORMS += \
    jlink_settings.ui \
    mainwindow.ui

TRANSLATIONS += \
    JLink_Flash_GUI_zh_CN.ts \
    JLink_Flash_GUI_en_US.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc

RC_ICONS = image/logo.ico

DISTFILES += \
    JLink_Flash_GUI_en_US.ts
