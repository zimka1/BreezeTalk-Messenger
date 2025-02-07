function handleChatList(users) {
    const chatItems = document.getElementById('chatItems');
    chatItems.innerHTML = '';
    users.forEach(user => {
        const chatItem = document.createElement('div');
        chatItem.className = 'chat-item';
        chatItem.dataset.userId = user.id;


        const userInfo = document.createElement('div');
        userInfo.className = 'user-info';
        userInfo.innerHTML = `
            <div class="user-name">${user.firstname} ${user.lastname} (ID: ${user.id})</div>
            <div class="last-message" id="last-message-${user.id}">Loading...</div>
        `;
        const userAvatar = document.createElement('div');
        userAvatar.className = 'user-avatar';
        userAvatar.innerHTML = user.firstname[0];



        chatItem.addEventListener('click', () => {
            showMessages();
            setActiveChat(chatItem);
            selectedUserId = Number(chatItem.dataset.userId);
            loadMessages(selectedUserId);
            appendCompanionInfInChat(`${user.firstname} ${user.lastname}`);
        });
        chatItem.appendChild(userAvatar);
        chatItem.appendChild(userInfo);
        chatItems.appendChild(chatItem);

        getLastMessageWithUser(user.id)

    });
}

function handleUserListRefresh() {
    const payload = JSON.stringify({command: "user_list_db"});
    socket.send(payload);
}

function handleLastMessage(parsedData) {
    let user_to= parsedData.user_to;
    let user_from = parsedData.user_from;
    let message = parsedData.message;
    let userFirstname_from = parsedData.userInfo_from.firstname;
    updateLastMessage(user_to, user_from, message, userFirstname_from);
}

function setActiveChat(chatItem) {
    const currentActive = document.querySelector('.chat-item.active-chat');
    if (currentActive) {
        currentActive.classList.remove('active-chat');
    }
    chatItem.classList.add('active-chat');
}

function getLastMessageWithUser(userId) {
    const payload = JSON.stringify({command: "get_last_message", user_id: userId});
    socket.send(payload);
}

function updateLastMessage(user_to, user_from, message, userFirstname_from) {
    let lastMessageElement;
    if (curUserID === user_to) {
        lastMessageElement = document.getElementById(`last-message-${user_from}`);
    } else if (curUserID === user_from) {
        lastMessageElement = document.getElementById(`last-message-${user_to}`);
    }

    if (message !== "") {
        lastMessageElement.innerHTML = userFirstname_from + ": " + message;
    } else {
        lastMessageElement.innerHTML = "";
    }
}