import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

Window {
    id: root
    visible: true
    width: 800
    height: 720
    title: "Qt Chat (TCP)"
    color: "#f2f6fc"

    property string current_user: ""
    property bool is_connected: false

    ListModel { id: online_user_list_model }

    Rectangle {
        width: parent.width
        height: 64
        anchors.top: parent.top
        color: "#5b8def"
        radius: 16
        z: 2

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 16
            Text {
                text: "💬 Qt Chat"
                font.pixelSize: 32
                font.bold: true
                color: "white"
                Layout.alignment: Qt.AlignVCenter
            }
            Rectangle { Layout.fillWidth: true; color: "transparent" }
        }
    }

    ColumnLayout {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 16
            topMargin: 80
        }
        spacing: 16

        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignHCenter

            TextField {
                id: host_field
                width: 200
                placeholderText: "host"
                text: "localhost"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            TextField {
                id: port_field
                width: 80
                placeholderText: "port"
                text: "9000"
                inputMethodHints: Qt.ImhDigitsOnly
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            Button {
                id: connect_btn
                text: is_connected ? "Disconnect" : "Connect"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#5b8def"; radius: 8 }
                contentItem: Text { text: connect_btn.text; color: "white"; font.pixelSize: 18 }
                onClicked: {
                    if (!is_connected) {
                        tcp_client.connect_to_host(host_field.text, parseInt(port_field.text))
                    } else {
                        tcp_client.disconnect_from_host()
                    }
                }
            }
            Label {
                id: status_label
                text: (!is_connected ? "Disconnected"
                    : (current_user.length > 0 ? "Login" : "Logout"))
                color: (!is_connected ? "darkred"
                    : (current_user.length > 0 ? "#158443" : "#fa9500"))
                verticalAlignment: Label.AlignVCenter
                font.bold: true
                font.pixelSize: 18
            }
        }

        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignHCenter

            TextField {
                id: username_field
                width: 200
                placeholderText: "username"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            TextField {
                id: password_field
                width: 200
                placeholderText: "password"
                echoMode: TextInput.Password
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            Button {
                id: register_open_btn
                text: "Register"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#66c6b8"; radius: 8 }
                contentItem: Text { text: "Register"; color: "white"; font.pixelSize: 18 }
                onClicked: register_dialog.open()
            }
            Button {
                id: login_btn
                text: current_user.length ? "Logged in" : "Login"
                enabled: !current_user.length
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#158443"; radius: 8 }
                contentItem: Text { text: login_btn.text; color: "white"; font.pixelSize: 18 }
                onClicked: {
                    tcp_client.send_json({ type: "login", username: username_field.text, password: password_field.text })
                }
            }
            Button {
                id: logout_btn
                visible: current_user.length > 0
                text: "Logout"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#FA9500"; radius: 8 }
                contentItem: Text { text: "Logout"; color: "white"; font.pixelSize: 18 }
                onClicked: {
                    tcp_client.send_json({ type: "logout", username: current_user });
                    current_user = "";
                    tcp_client.send_json({ type: "list_users" });
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text { text: "Messages in view: " + (list_view ? list_view.count : 0); font.bold: true; font.pixelSize: 16; color: "#5b8def" }
            Text { text: "Online: " + online_user_list_model.count; font.pixelSize: 16; color: "#158443" }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#ffffff"
                radius: 16
                border.color: "#5b8def"

                ListView {
                    id: list_view
                    anchors.fill: parent
                    anchors.margins: 10
                    model: message_model
                    clip: true
                    spacing: 8
                    highlightFollowsCurrentItem: true

                    delegate: Rectangle {
                        property string body: text
                        visible: !((sender === root.current_user || sender === "me") && index > 0 && message_model.get(index-1).sender === "me")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 8
                        radius: 10
                        border.color: "#e6edfa"
                        color: (sender === root.current_user || sender === "me") ? "#e6f7ff" : "#f7f9fd"
                        implicitHeight: content_column.implicitHeight + 16

                        Column {
                            id: content_column
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Row {
                                spacing: 8
                                Text { text: sender; font.bold: true; elide: Text.ElideRight; color: "#5b8def"; font.pixelSize: 16 }
                                Text { text: time; color: "#888888"; font.pointSize: 12 }
                            }
                            Text {
                                text: body
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignLeft
                                color: "#2E4A62"
                                font.pixelSize: 18
                            }
                        }
                    }

                    onCountChanged: {
                        if (count > 0) positionViewAtEnd()
                    }
                }
            }

            Rectangle {
                width: 200
                color: "#f9f9ff"
                radius: 14
                border.color: "#66c6b8"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        text: (is_connected && current_user.length == 0)
                            ? "Online Users Number" : "Online Users"
                        font.bold: true
                        font.pixelSize: 18
                        color: "#158443"
                    }

                    ListView {
                        id: users_view
                        model: online_user_list_model
                        clip: true
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: (is_connected && current_user.length > 0)
                        delegate: Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 32
                            radius: 6
                            color: ListView.isCurrentItem ? "#e6f7ff" : "transparent"
                            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 6; text: name; font.pixelSize: 16 }
                        }
                    }
                    Item {
                        visible: (is_connected && current_user.length == 0)
                        width: parent.width
                        height: 32
                        Row {
                            spacing: 6
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            Text { text: "Online user count:"; font.pixelSize: 16; color: "#888" }
                            Text { text: online_user_list_model.count; font.pixelSize: 16; color: "#66c6b8" }
                        }
                    }
                }
            }
        }

        RowLayout {
            id: input_row
            spacing: 12
            Layout.fillWidth: true
            height: 60

            Rectangle {
                Layout.fillWidth: true
                color: "#ffffff"
                radius: 12
                border.color: "#e6edfa"
                anchors.verticalCenter: parent.verticalCenter
                height: 54

                TextField {
                    id: input_field
                    placeholderText: "Type a message..."
                    focus: true
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    font.pixelSize: 20
                    height: 54
                    background: null
                    onAccepted: send_btn.clicked()
                }
            }
            Button {
                id: send_btn
                text: "Send"
                height: 54
                font.pixelSize: 20
                background: Rectangle { color: "#5b8def"; radius: 12 }
                contentItem: Text { text: "Send"; color: "white"; font.pixelSize: 20 }
                onClicked: {
                    if (input_field.text.length > 0) {
                        if (is_connected && current_user.length == 0) {
                            message_model.add_message("me", input_field.text, new Date());
                            not_seen_warning_dialog.open();
                            input_field.text = "";
                            return;
                        }
                        if (is_connected && current_user.length > 0) {
                            tcp_client.send_json({ type: "message", text: input_field.text });
                            input_field.text = "";
                        }
                    }
                }
            }
        }

        Label {
            id: not_seen_label
            visible: (is_connected && current_user.length == 0)
            color: "#e39d26"
            font.bold: true
            font.pixelSize: 16
            text: "Notice: Messages sent when logged out or after logout will NOT be visible to other online users."
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Dialog {
        id: register_ok_dialog
        title: "Register"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Register successful!"; font.bold: true; color: "#158443"; font.pixelSize: 20 }
    }
    Dialog {
        id: already_registered_dialog
        title: "Register Failed"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Already registered!"; font.bold: true; color: "#a94442"; font.pixelSize: 20 }
    }
    Dialog {
        id: login_fail_dialog
        title: "Login Failed"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Login is invalid."; font.bold: true; color: "#a94442"; font.pixelSize: 20 }
    }
    Dialog {
        id: login_ok_dialog
        title: "Welcome!"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Login successful!"; font.bold: true; color: "#158443"; font.pixelSize: 20 }
    }
    Dialog {
        id: not_seen_warning_dialog
        title: "Notice"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Notice: Messages sent when logged out or after logout will NOT be visible to other online users."; font.bold: true; color: "#e39d26"; font.pixelSize: 20 }
    }
    Dialog {
        id: register_dialog
        title: "Register"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            tcp_client.send_json({ type: "register", username: register_username.text, password: register_password.text })
        }
        contentItem: ColumnLayout {
            spacing: 10
            TextField { id: register_username; placeholderText: "username"; font.pixelSize: 16; height: 40; background: Rectangle{ color: "#e6edfa"; radius: 8 } }
            TextField { id: register_password; placeholderText: "password"; echoMode: TextInput.Password; font.pixelSize: 16; height: 40; background: Rectangle{ color: "#e6edfa"; radius: 8 } }
        }
    }

    Connections {
        target: tcp_client

        function onConnected() {
            is_connected = true;
            tcp_client.send_json({ type: "list_users" });
        }
        function onDisconnected() {
            is_connected = false;
            online_user_list_model.clear();
            current_user = "";
            tcp_client.send_json({ type: "list_users" });
        }
        function onLogin_succeeded(username) {
            if (username && username.length > 0)
                current_user = username;
            else
                current_user = username_field.text;

            password_field.text = "";
            tcp_client.send_json({ type: "list_users" });
            login_ok_dialog.open();
        }
        function onLogin_failed(reason) {
            login_fail_dialog.open();
        }
        function onRegister_succeeded() {
            register_dialog.close();
            register_ok_dialog.open();
        }
        function onRegister_failed(reason) {
            if (reason === "already registered"
                || reason === "username_exists"
                || reason.indexOf("exists") > -1
                || reason.indexOf("registered") > -1)
            {
                already_registered_dialog.open();
            }
        }
        function onOnline_users_updated(users) {
            online_user_list_model.clear();
            for (var i=0; i<users.length; i++) {
                online_user_list_model.append({ "name": users[i] });
            }
        }
        function onMessage_received(from, text, ts) {
            if (is_connected && current_user.length == 0) return;
            if (list_view.count > 0) list_view.positionViewAtEnd();
        }
    }
}