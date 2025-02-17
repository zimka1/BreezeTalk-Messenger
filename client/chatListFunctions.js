function handleChatList(users) {
    const chatItems = document.getElementById('chatItems');
    chatItems.innerHTML = '';
    users.forEach(user => {
        const chatItem = document.createElement('div');
        chatItem.className = 'chat-item';
        chatItem.dataset.userId = user.id;

        const userAvatar = document.createElement('div');
        userAvatar.className = 'user-avatar';
        userAvatar.innerHTML = user.firstname[0];

        const userInfo = document.createElement('div');
        userInfo.className = 'user-info';
        userInfo.innerHTML = `
            <div class="user-name">${user.firstname} ${user.lastname} (ID: ${user.id})</div>
        `;

        const messagesInfo= document.createElement('div');
        messagesInfo.className = 'messages-info';
        messagesInfo.innerHTML = `
            <div class="last-message" id="last-message-${user.id}">Loading...</div>
            <div class="unreadMessagesCount" id="unreadMessagesCount-${user.id}" style="display: none"></div>
        `;

        chatItem.addEventListener('click', () => {
            showMessages();
            setActiveChat(chatItem);
            selectedUserId = Number(chatItem.dataset.userId);
            const unreadMessagesCount = document.getElementById(`unreadMessagesCount-${selectedUserId}`);
            if (unreadMessagesCount.style.display === "block") {
                loadMessages(selectedUserId, 1);
            } else {
                loadMessages(selectedUserId, 0);
            }
            appendCompanionInfInChat(`${user.firstname} ${user.lastname}`);
            document.getElementById(`unreadMessagesCount-${selectedUserId}`).style.display = "none";
        });

        chatItem.appendChild(userAvatar);

        userInfo.appendChild(messagesInfo);
        chatItem.appendChild(userInfo);

        chatItems.appendChild(chatItem);

        getUnreadMessagesCount(user.id);
        getLastMessageWithUser(user.id);

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

function handleUnreadMessagesCount(parsedData) {
    if (parsedData.messagesCount) {
        document.getElementById(`unreadMessagesCount-${parsedData.user_id}`).style.display = "block";
        document.getElementById(`unreadMessagesCount-${parsedData.user_id}`).innerHTML = String(parsedData.messagesCount);
    }
}

function setActiveChat(chatItem) {
    const currentActive = document.querySelector('.chat-item.active-chat');
    if (currentActive) {
        currentActive.classList.remove('active-chat');
    }
    chatItem.classList.add('active-chat');
}

function getUnreadMessagesCount(userId) {
    const payload = JSON.stringify({command: "get_unread_messages_count", user_id: userId});
    socket.send(payload);
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