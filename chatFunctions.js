function handleMessages(parsedData) {
    document.getElementById('messages').innerHTML = '';
    parsedData.messages.forEach(msg => appendMessage(`${msg.from}`,`${msg.message}`));
}

function handlePrivateMessage(parsedData){
    if (selectedUserId === parsedData.user_from){
        loadMessages(parsedData.user_from);
    }
    let user_from = parsedData.user_from;
    getLastMessageWithUser(user_from);
}

document.getElementById('sendMessageButton').addEventListener('click', () => {
    const message = document.getElementById('messageInput').value;

    if (message.trim() !== "" && selectedUserId !== null) {
        const payload = JSON.stringify({command: "private_msg", text: message, user_from: curUserID, user_to: selectedUserId});
        socket.send(payload);
        document.getElementById('messageInput').value = "";
        loadMessages(selectedUserId)
        getLastMessageWithUser(selectedUserId);
    }
});

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

function loadMessages(userId) {
    const payload = JSON.stringify({command: "get_messages", user_id: userId});
    socket.send(payload);
}