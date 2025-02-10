function handleOnlyReadMessages(parsedData) {
    document.getElementById('messages').innerHTML = '';
    parsedData.messages.forEach(msg => appendMessage(`${msg.from}`,`${msg.message}`));
}

function handleOnlyUnreadMessages(parsedData) {
    if (parsedData.messages.length) {
        const dividingLine = document.createElement('div');
        dividingLine.textContent = "Unread Messages";
        dividingLine.className = "dividingLine";
        document.getElementById('messages').appendChild(dividingLine);
    }
    parsedData.messages.forEach(msg => appendUnreadMessage(`${msg.from}`,`${msg.message}`));
}

function handlePrivateMessage(parsedData){
    if (selectedUserId === parsedData.user_from){
        sendMessageToReadMessagesTable(parsedData, 1);
        loadMessages(parsedData.user_from, 0);
    } else {
        sendMessageToReadMessagesTable(parsedData, 0);
    }
    let user_from = parsedData.user_from;
    getLastMessageWithUser(user_from);
}

function handleSentAcknowledgement(parsedData) {
    console.log("aaaaaaaaaa");
    if (selectedUserId === parsedData.user_to) {
        console.log(parsedData.user_to);
        loadMessages(parsedData.user_to, 0);
    }
    getLastMessageWithUser(parsedData.user_to);
}

function sendMessageToReadMessagesTable(parsedData, hasBeenRead) {
    const payload = JSON.stringify({command: "save_read_private_msg", message: parsedData.message, user_from: parsedData.user_from, user_to: parsedData.user_to, hasBeenRead: hasBeenRead});
    socket.send(payload);
}

document.getElementById('sendMessageButton').addEventListener('click', () => {
    const message = document.getElementById('messageInput').value;

    if (message.trim() !== "" && selectedUserId !== null) {
        const payload = JSON.stringify({command: "private_msg", message: message, user_from: curUserID, user_to: selectedUserId});
        socket.send(payload);
        document.getElementById('messageInput').value = "";
    }
});

function appendUnreadMessage(userFromID, message) {
    const messageDiv = document.createElement('div');
    messageDiv.textContent = message;
    if (userFromID == curUserID) {
        messageDiv.className = "message curUser";
    } else {
        messageDiv.className = "message"
    }
    document.getElementById('messages').appendChild(messageDiv);
    const messagesContainer = document.getElementById('messages');
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
}

function appendMessage(userFromID, message) {
    const messageDiv = document.createElement('div');
    messageDiv.textContent = message;
    if (userFromID == curUserID) {
        messageDiv.className = "message curUser";
    } else {
        messageDiv.className = "message"
    }
    document.getElementById('messages').appendChild(messageDiv);
    const messagesContainer = document.getElementById('messages');
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
}

function appendCompanionInfInChat(userName){
    document.getElementById('companionInfInChat').innerHTML = '';
    const infDiv = document.createElement('div');
    infDiv.textContent = userName;
    document.getElementById('companionInfInChat').appendChild(infDiv);
}

function loadMessages(userId, markMessagesAsRead) {
    const payload = JSON.stringify({command: "get_messages", user_id: userId, markMessagesAsRead: markMessagesAsRead});
    socket.send(payload);
}

