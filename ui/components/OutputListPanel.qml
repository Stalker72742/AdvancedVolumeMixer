import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

Rectangle {
    id: root
    color: Theme.bgSecondary

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.pagePadding
        anchors.bottomMargin: Theme.spacingMedium
        anchors.leftMargin: Theme.spacingMedium
        anchors.rightMargin: Theme.spacingMedium
        spacing: Theme.spacingMedium

        SectionLabel {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            text: "Audio outputs"
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds

            model: Mixer.outputs

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: OutputListItem {
                required property int index
                required property string name
                required property bool online
                required property bool isDefault
                required property string activeProfileName

                width: ListView.view.width
                outputName: name
                isOnline: online
                outputSubtitle: !online ? "Offline"
                              : (isDefault ? "System default · " : "") + activeProfileName
                selected: index === Mixer.currentOutputIndex
                canForget: !online

                onClicked: Mixer.selectOutput(index)
                onForgetRequested: Mixer.forgetOutput(index)
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                visible: listView.count === 0
                text: "No audio outputs found"
                color: Theme.textDisabled
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }
}
