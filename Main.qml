import QtQuick
import VideoPlayer
import QtQuick.Controls
import Qt.labs.platform

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("VPlayer")

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
        onPlayTime: time => {
            const curTime = Math.floor(time / 1000000)
            playTime.text = formatTime(curTime)
            playBar.value = curTime
        }
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
            videoPlayer.play()
        }
    }

    Button {
        id: stop
        text: '暂停'
        x: 100
        onClicked: {
            videoPlayer.stop()
        }
    }

    Button {
        text: '前进'
        x: 100
        y: 50
        onClicked: {
            videoPlayer.seekToPosition(2000)
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
        id: playTime
        text: '00:00:00'
        color: '#FFFFFF'
        x: 150
    }

    Text {
        id: totleTime
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
        onValueChanged: {
            console.log('value Change: ', this.value)
        }
        onPressedChanged: {
            videoPlayer.seekToPosition(this.value)
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
                totleTime.text = formatTime(time)
                playBar.to = time
            }
        }
    }
}
