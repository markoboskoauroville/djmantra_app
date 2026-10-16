import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// MANUAL (V86, Marko 16.10.2026: "create manual for this app explaining technology behind it and how to
// use it with screenshot ... The user can see it with MD reader you're going to build for this app ...
// export of that MD or PDF, both available in the app itself. So user can print the manual"). The manual
// is manual/MANUAL.md next to this file with its pictures in manual/images. Read in blocks: a picture on
// its own line ("![caption](images/x.png)") is shown the screen's width, everything else as Markdown
// (headings, lists, tables, bold). EXPORT MD / EXPORT PDF write it to Documents/DJ Mantra/Manual.
Rectangle {
    id: root

    signal done()

    readonly property url folder: Qt.resolvedUrl("manual/")
    readonly property string text: Mixxx.DJMantra.readTextFile(Qt.resolvedUrl("manual/MANUAL.md"))
    readonly property var blocks: {
        const out = [];
        const image = /^!\[([^\]]*)\]\(([^)\s]+)\)\s*$/;
        for (const part of text.split(/\n\s*\n/)) {
            const t = part.trim();
            if (t === "")
                continue;
            const m = t.match(image);
            if (m)
                out.push({kind: "image", caption: m[1], source: root.folder + m[2]});
            else
                out.push({kind: "text", text: t});
        }
        return out;
    }

    function exportAs(format) {
        const path = Mixxx.DJMantra.exportManual(format, root.folder);
        Mixxx.DJMantra.log("Manual: export " + format + " -> " + path);
        if (path === "")
            Toasts.show(qsTr("MANUAL: EXPORT FAILED"));
    }

    color: Style.background

    MouseArea {
        anchors.fill: parent
    }

    Item {
        id: head

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        height: 44

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("MANUAL")
            color: Style.accent
            font.pixelSize: 18
            font.bold: true
        }

        Row {
            anchors.right: close.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            DButton {
                objectName: "manualExportMd"
                width: 96
                height: 40
                mode: ""
                text: qsTr("EXPORT MD")
                fontSize: 12
                onClicked: root.exportAs("md")
            }
            DButton {
                objectName: "manualExportPdf"
                width: 96
                height: 40
                mode: ""
                text: qsTr("EXPORT PDF")
                fontSize: 12
                lit: true
                onClicked: root.exportAs("pdf")
            }
        }

        CloseButton {
            id: close

            objectName: "manualClose"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            onClicked: root.done()
        }
    }

    ListView {
        id: list

        objectName: "manualText"
        anchors.top: head.bottom
        anchors.topMargin: 8
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        clip: true
        spacing: 10
        model: root.blocks
        cacheBuffer: 2000

        delegate: Loader {
            required property var modelData

            width: list.width
            sourceComponent: modelData.kind === "image" ? imageBlock : textBlock

            Component {
                id: textBlock

                Text {
                    width: list.width
                    text: modelData.text
                    textFormat: Text.MarkdownText
                    wrapMode: Text.Wrap
                    color: Style.text
                    linkColor: Style.accent
                    font.pixelSize: 15
                    onLinkActivated: (link) => Qt.openUrlExternally(link)
                }
            }

            Component {
                id: imageBlock

                Column {
                    width: list.width
                    spacing: 4

                    Image {
                        // a phone screen at most 60 % of the width, wide pictures the full width
                        width: implicitWidth > implicitHeight ? list.width : Math.min(list.width * 0.6, implicitWidth)
                        anchors.horizontalCenter: parent.horizontalCenter
                        fillMode: Image.PreserveAspectFit
                        source: modelData.source
                        asynchronous: true
                        smooth: true
                        mipmap: true
                    }
                    Text {
                        width: list.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: modelData.caption
                        color: Style.subText
                        font.pixelSize: 12
                        font.italic: true
                    }
                }
            }
        }
    }
}
