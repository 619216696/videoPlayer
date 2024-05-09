import QtQuick
import VideoPlayer
import QtQuick.Controls
import Qt.labs.platform

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("VPlayer")
    color: 'black'

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

    VideoPlayer {
        id: videoPlayer
        height: parent.height
        width: parent.width
    }

    Button {
        id: open
        text: '打开'
        onClicked: {
            fileDialog.open()
        }
    }

    Button {
        id: play
        text: '播放'
        x: 50
        onClicked: {
            videoPlayer.setPlayState(true)
            playTimeTimer.start()
        }
    }

    Button {
        id: stop
        text: '暂停'
        x: 100
        onClicked: {
            videoPlayer.setPlayState(false)
            playTimeTimer.stop()
        }
    }

    Button {
        text: '前进'
        x: 100
        y: 50
        onClicked: {
            videoPlayer.seekToPosition(1977)
        }
    }

    Button {
        text: '后退'
        x: 150
        y: 50
        onClicked: {
            videoPlayer.seekToPosition(1000)
        }
    }

    Text {
        id: playTimeText
        text: '00:00:00'
        color: '#FFFFFF'
        x: 150
    }

    Text {
        id: totleTimeText
        text: '00:00:00'
        color: '#FFFFFF'
        x: 250
    }

    Slider {
        id: playBar
        anchors.left: parent.left
        anchors.right: parent.right
        x: 20
        y: 30
        from: 0 // 设置滑块的最小值
        to: 0 // 设置滑块的最大值
        value: 0 // 设置滑块的当前值
        onPressedChanged: {
            if (videoPlayer.getPlayTime() === this.value) return
            playTimeTimer.stop()
            const second = Math.floor(this.value)
            console.log('seekToPosition: ', second)
            videoPlayer.seekToPosition(second)
            playTimeTimer.start()
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
            }
        }
    }
}
