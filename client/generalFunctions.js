let socket;
let curUserID;
let selectedUserId = null;


document.getElementById('connectButton').addEventListener('click', connectToWebSocket);

function connectToWebSocket() {
    socket = new WebSocket('ws://127.0.0.1:9001/');

    socket.onopen = handleSocketOpen;
    socket.onmessage = handleSocketMessage;
    socket.onclose = handleSocketClose;
    socket.onerror = handleSocketError;
}

function handleSocketOpen(event) {
    console.log("Connected to server");
    document.getElementById('connectionStatus').innerText = "Connected";
    document.getElementById('connectButton').style.display = 'none';
    document.getElementById('enterID').style.display = 'flex';
    document.getElementById('login').style.display = 'flex';
}

function handleSocketMessage(event) {
    console.log(">server>", event.data);

    if (typeof event.data === 'string') {
        try {
            const parsedData = JSON.parse(event.data);
            handleParsedData(parsedData);
        } catch (e) {
            console.log(event.data);
        }
    }
}

function handleParsedData(parsedData) {
    if (parsedData.command === "registered" && parsedData.user_id !== undefined) {
        handleRegistered(parsedData);
    } else if (parsedData.command === "logged_in" && parsedData.user_id !== undefined) {
        handleLoggedIn(parsedData);
    } else if (parsedData.command === "login_failed") {
        handleLoginFail(parsedData);
    } else if (parsedData.command === "user_list") {
        handleChatList(parsedData.users);
    } else if (parsedData.command === "user_list_refresh") {
        handleUserListRefresh();
    } else if (parsedData.command === "onlyReadMessages") {
        handleOnlyReadMessages(parsedData);
    } else if (parsedData.command == "onlyUnreadMessages") {
        handleOnlyUnreadMessages(parsedData);
    } else if (parsedData.command === "private_msg") {
        handlePrivateMessage(parsedData);
    } else if (parsedData.command === "last_message") {
        handleLastMessage(parsedData);
    } else if (parsedData.command === "unreadMessagesCount") {
        handleUnreadMessagesCount(parsedData);
    } else if (parsedData.command === "sentAcknowledgement") {
        handleSentAcknowledgement(parsedData);
    }
}

function handleSocketClose(event) {
    console.log("Disconnected from server");
    document.getElementById('connectionStatus').innerText = "Disconnected";
}

function handleSocketError(error) {
    console.error("WebSocket error: ", error);
    document.getElementById('connectionStatus').innerText = "Error";
}


document.getElementById('creatNewAccount').addEventListener('click', () => {
    document.getElementById('login').style.display = 'none';
    document.getElementById('registration').style.display = 'flex';
});

document.getElementById('alreadyHaveAccount').addEventListener('click', () => {
    document.getElementById('registration').style.display = 'none';
    document.getElementById('login').style.display = 'flex';
});

document.getElementById('logoutButton').addEventListener('click', () => {
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.close();
    }
    hideChatElements();
    const payload = JSON.stringify({command: "logout"});
    socket.send(payload);
});

function showChatElements() {
    document.getElementById('enterID').style.display = 'none';
    document.getElementById('content').style.display = 'flex'
    document.getElementById('header').style.display = 'flex'
    const payload = JSON.stringify({command: "user_list_db"});
    socket.send(payload);
}

function showMessages() {
    document.getElementById('companionInfInChat').style.display = 'block';
    document.getElementById('messages').style.display = 'flex';
    document.getElementById('messageInputContainer').style.display = 'flex';

}

function hideChatElements() {
    document.getElementById('enterID').style.display = 'none';
    document.getElementById('login').style.display = 'none';
    document.getElementById('registration').style.display = 'none';
    document.getElementById('content').style.display = 'none'
    document.getElementById('header').style.display = 'none'
    document.getElementById('companionInfInChat').style.display = 'none';
    document.getElementById('messages').style.display = 'none';
    document.getElementById('messageInputContainer').style.display = 'none';
    document.getElementById('connectButton').style.display = 'block';
}
