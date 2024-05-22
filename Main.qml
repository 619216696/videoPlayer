import QtQuick
import VideoPlayer
import QtQuick.Controls
import Qt.labs.platform
import QtQuick.Layouts 1.2

Window {
    id: mainWindow
    width: 640
    height: 480
    visible: true
    title: qsTr("VPlayer")
    color: 'black'
    flags: Qt.Window | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint

    // 创建一个定时器，用来获取播放时间，间隔500ms获取一次
    Timer {
        id: playTimeTimer
        interval: 500
        repeat: true
        running: false

        onTriggered: {
           const playTime = videoPlayer.getPlayTime()
           playTimeText.text = formatTime(playTime)
           playBar.value = playTime
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        onPositionChanged: (mouse) => {
            // 标题栏显示隐藏
            if (mouse.y <= titleBar.height) {
                titleBar.opacity = 1
            } else {
                titleBar.opacity = 0
            }

            // 控制栏显示隐藏
            if (mouse.y >= controlBar.y) {
               controlBar.opacity = 1
            } else {
               controlBar.opacity = 0
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "打开视频"
        nameFilters: ["*.mp4", "*.avi", "*.mkv", "*"]
        onAccepted: {
            console.log("Selected file: " + fileDialog.file)
            if (videoPlayer.loadVideo(fileDialog.file, false)) {
                const time = videoPlayer.getVideoTotleTime()
                totleTimeText.text = formatTime(time)
                playBar.to = time
                // 启动定时器，获取播放时间
                playTimeTimer.start()
                btnPlay.enabled = true
                playBar.enabled = true
                volumnBar.enabled = true
            }
        }
    }

    // 格式化时间
    function formatTime(seconds) {
        const hours = Math.floor(seconds / 3600)
        const minutes = Math.floor((seconds % 3600) / 60)
        const remainingSeconds = seconds % 60

        const formattedHours = String(hours).padStart(2, '0')
        const formattedMinutes = String(minutes).padStart(2, '0')
        const formattedSeconds = String(remainingSeconds).padStart(2, '0')

        return `${formattedHours}:${formattedMinutes}:${formattedSeconds}`
    }

    // 视频播放组件
    VideoPlayer {
        id: videoPlayer
        height: parent.height
        width: parent.width
    }

    // 顶部栏
    Rectangle {
        id: titleBar
        width: parent.width
        height: 30
        color: "#16000000"  // 黑色背景，20%透明度
        opacity: 1  // 默认显示

        Behavior on opacity {
            OpacityAnimator {
                duration: 1000  // 淡出效果持续时间
            }
        }

        // 打开文件
        Rectangle {
            width: 16
            height: 16
            color: "transparent"
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.top: parent.top
            anchors.topMargin: 6

            ToolTip {
                id: openFileToolTip
                text: "打开文件"
            }

            Image {
                anchors.centerIn: parent
                source: "qrc:/assets/icon_file.svg"
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onClicked: fileDialog.open()
                onEntered: {
                    openFileToolTipTimer.start()
                }
                onExited: {
                    openFileToolTip.visible = false
                    openFileToolTipTimer.stop()
                }
            }

            Timer {
                id: openFileToolTipTimer
                interval: 500
                repeat: false
                running: false

                onTriggered: {
                   openFileToolTip.visible = true
                   openFileToolTipTimer.stop()
                }
            }
        }

        RowLayout {
            spacing: 5
            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.top: parent.top
            anchors.topMargin: 4

            // 自定义最小化按钮
            Rectangle {
                width: 16
                height: 16
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: "qrc:/assets/icon_minimize.svg"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: mainWindow.showMinimized()
                }
            }

            // 自定义最大化/还原按钮
            Rectangle {
                width: 16
                height: 16
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: mainWindow.visibility === Window.Maximized ? "qrc:/assets/icon_restore.svg" : "qrc:/assets/icon_maximize.svg"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (mainWindow.visibility === Window.Maximized) {
                            mainWindow.showNormal()
                        } else {
                            mainWindow.showMaximized()
                        }
                    }
                }
            }

            // 全屏按钮
            Rectangle {
                width: 16
                height: 16
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: mainWindow.visibility === Window.FullScreen ? "qrc:/assets/icon_fullScreenExit.svg" : "qrc:/assets/icon_fullScreen.svg"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (mainWindow.visibility === Window.FullScreen) {
                            mainWindow.visibility = Window.Windowed
                        } else {
                            mainWindow.visibility = Window.FullScreen
                        }
                    }
                }
            }

            // 自定义关闭按钮
            Rectangle {
                width: 16
                height: 16
                color: "transparent"
                Image {
                    anchors.centerIn: parent
                    source: "qrc:/assets/icon_close.svg"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: mainWindow.close()
                }
            }
        }
    }

    // 底部控制栏
    Rectangle {
        id: controlBar
        width: parent.width
        height: 60
        color: "#16000000"  // 黑色背景，20%透明度
        anchors.bottom: parent.bottom
        opacity: 1  // 默认显示

        Behavior on opacity {
            OpacityAnimator {
                duration: 1000  // 淡出效果持续时间
            }
        }

        Text {
            id: playTimeText
            text: '00:00:00'
            color: '#FFFFFF'
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 80
            anchors.bottomMargin: 8
        }

        Text {
            id: totleTimeText
            text: '00:00:00'
            color: '#FFFFFF'
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: volumnBar.availableWidth + 44
            anchors.bottomMargin: 8
        }

        RowLayout {
            width: parent.width
            height: parent.height

            Image {
                id: btnPlay
                enabled: false
                width: 36
                height: 36
                source: videoPlayer.playing ? 'qrc:/assets/icon_stop.svg' : 'qrc:/assets/icon_play.svg'
                Layout.leftMargin: 32

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (videoPlayer.playing) {
                            console.log('click puase')
                            videoPlayer.setPlayState(false)
                        } else {
                            console.log('click play')
                            videoPlayer.setPlayState(true)
                        }
                    }
                }
            }

            Slider {
                id: playBar
                enabled: false
                Layout.leftMargin: 8
                Layout.fillWidth: true
                from: 0 // 设置滑块的最小值
                to: 0 // 设置滑块的最大值
                value: 0 // 设置滑块的当前值
                background: Rectangle {
                    x: playBar.leftPadding
                    y: playBar.topPadding + playBar.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: playBar.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#797a7b"

                    Rectangle {
                        width: playBar.visualPosition * parent.width
                        height: parent.height
                        color: "#edeeee"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: playBar.leftPadding + playBar.visualPosition * (playBar.availableWidth - width)
                    y: playBar.topPadding + playBar.availableHeight / 2 - height / 2
                    implicitWidth: 12
                    implicitHeight: 12
                    radius: 6
                }

                onMoved: {
                    if (videoPlayer.getPlayTime() === this.value) return
                    const second = Math.floor(this.valueAt(this.position))
                    console.log('seekToPosition: ', second)
                    videoPlayer.seekToPosition(second)
                    playTimeTimer.start()
                }
            }

            Slider {
                id: volumnBar
                enabled: false
                Layout.leftMargin: 8
                Layout.rightMargin: 32
                from: 0 // 设置滑块的最小值
                to: 100 // 设置滑块的最大值
                value: videoPlayer.volumn // 设置滑块的当前值
                background: Rectangle {
                    x: volumnBar.leftPadding
                    y: volumnBar.topPadding + volumnBar.availableHeight / 2 - height / 2
                    implicitWidth: 40
                    implicitHeight: 4
                    width: volumnBar.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#797a7b"

                    Rectangle {
                        width: volumnBar.visualPosition * parent.width
                        height: parent.height
                        color: "#edeeee"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: volumnBar.leftPadding + volumnBar.visualPosition * (volumnBar.availableWidth - width)
                    y: volumnBar.topPadding + volumnBar.availableHeight / 2 - height / 2
                    implicitWidth: 12
                    implicitHeight: 12
                    radius: 6
                }

                onMoved: {
                    videoPlayer.volumn = this.value
                }
            }
        }
    }
}
