import QtQuick
import QtQuick.Controls
import AdvancedVolumeMixer.ThemeModule

// Horizontal strip of the current output's profiles.
Item {
    id: root

    readonly property int cardHeight: 84

    // Room below the cards for the horizontal scrollbar
    implicitHeight: cardHeight + 12

    ListView {
        id: listView
        anchors.fill: parent
        orientation: ListView.Horizontal
        spacing: Theme.spacingMedium
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        model: Mixer.profiles

        delegate: ProfileCard {
            required property int index
            required property string name
            required property bool isActive

            height: root.cardHeight
            profileName: name
            active: isActive
            selected: index === Mixer.currentProfileIndex
            onClicked: Mixer.selectProfile(index)
        }

        // Footer has no automatic spacing before it, add it by hand
        footer: Item {
            width: addCard.width + listView.spacing
            height: root.cardHeight

            ProfileCard {
                id: addCard
                x: listView.spacing
                height: root.cardHeight
                isAddCard: true
                onClicked: Mixer.addProfile()
            }
        }

        ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
    }
}
