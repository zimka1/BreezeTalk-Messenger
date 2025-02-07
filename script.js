let socket;
let curUserID;
let selectedUserId = null;

document.getElementById('connectButton').addEventListener('click', () => {
    socket = new WebSocket('ws://127.0.0.1:9001/');

    socket.onopen = function(event) {
        console.log("Connected to server");
        appendMessage("Connected to server");
        document.getElementById('connectionStatus').innerText = "Connected";
        document.getElementById('connectButton').style.display = 'none';
        document.getElementById('enterID').style.display = 'flex';
        document.getElementById('login').style.display = 'flex';
    };

    socket.onmessage = function(event) {
        console.log(">server>", event.data);

        if (typeof event.data === 'string') {
            try {
                const parsedData = JSON.parse(event.data);
                console.log(parsedData)

                if (parsedData.command === "registered" && parsedData.user_id !== undefined) {
                    document.getElementById('connectionStatus').innerText = "Logged in as " + parsedData.name + " (User ID: " + parsedData.user_id + ")";
                    curUserID = parsedData.user_id;
                    showChatElements();
                } else if (parsedData.command === "logged_in" && parsedData.user_id !== undefined) {
                    document.getElementById('connectionStatus').innerText = "Logged in as " + parsedData.name + " (User ID: " + parsedData.user_id + ")";
                    curUserID = parsedData.user_id;
                    showChatElements();
                } else if (parsedData.command === "login_failed") {
                    appendMessage("Login failed: " + parsedData.message);
                } else if (parsedData.command === "user_list") {
                    updateChatList(parsedData.users);
                } else if (parsedData.command === "user_list_refresh") {
                    const payload = JSON.stringify({command: "user_list_db"});
                    socket.send(payload);
                } else if (parsedData.command === "messages") {
                    document.getElementById('messages').innerHTML = '';
                    parsedData.messages.forEach(msg => appendMessage(`${msg.from}`,`${msg.message}`));
                } else if (parsedData.command === "private_msg") {
                    if (selectedUserId === parsedData.user_from){
                        loadMessages(parsedData.user_from);
                    }
                    let user_from = parsedData.user_from;
                    getLastMessageWithUser(user_from);
                } else if (parsedData.command === "last_message") {
                    let user_to= parsedData.user_to;
                    let user_from = parsedData.user_from;
                    let message = parsedData.message;
                    let userFirstname_from = parsedData.userInfo_from.firstname;
                    updateLastMessage(user_to, user_from, message, userFirstname_from);

                }
            } catch (e) {
                appendMessage(event.data);
            }
        } else {
            appendMessage("Received non-text message");
        }
    };

    socket.onclose = function(event) {
        console.log("Disconnected from server");
        appendMessage("Disconnected from server");
        document.getElementById('connectionStatus').innerText = "Disconnected";
    };

    socket.onerror = function(error) {
        console.error("WebSocket error: ", error);
        appendMessage("WebSocket error: " + error.message);
        document.getElementById('connectionStatus').innerText = "Error";
    };
});


document.getElementById('creatNewAccount').addEventListener('click', () => {
    document.getElementById('login').style.display = 'none';
    document.getElementById('registration').style.display = 'flex';
});

document.getElementById('alreadyHaveAccount').addEventListener('click', () => {
    document.getElementById('registration').style.display = 'none';
    document.getElementById('login').style.display = 'flex';
});

document.getElementById('registerSubmitButton').addEventListener('click', () => {
    const firstname = document.getElementById('firstNameInput').value;
    const lastname = document.getElementById('lastNameInput').value;
    const nickname = document.getElementById('regNameInput').value;
    const password = document.getElementById('regPasswordInput').value;

    document.getElementById('regNameInput').value = "";
    document.getElementById('regPasswordInput').value = "";
    document.getElementById('firstNameInput').value = "";
    document.getElementById('lastNameInput').value = "";

    console.log(firstname);
    console.log(lastname);
    console.log(nickname);
    console.log(password);

    if (nickname.trim() !== "" && password.trim() !== "") {
        const payload = JSON.stringify({command: "register", firstname: firstname, lastname: lastname, nickname: nickname, password: password});
        socket.send(payload);
    }
});

document.getElementById('loginSubmitButton').addEventListener('click', () => {
    const nickname = document.getElementById('loginNameInput').value;
    const password = document.getElementById('loginPasswordInput').value;

    document.getElementById('loginNameInput').value = "";
    document.getElementById('loginPasswordInput').value = "";

    if (nickname.trim() !== "" && password.trim() !== "") {
        const payload = JSON.stringify({command: "login", nickname: nickname, password: password});
        socket.send(payload);
    }
});

document.getElementById('logoutButton').addEventListener('click', () => {
    hideChatElements();
    const payload = JSON.stringify({command: "logout"});
    socket.send(payload);
});

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


function updateChatList(users) {
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

        chatItem.addEventListener('click', () => {
            showMessages();
            setActiveChat(chatItem);
            selectedUserId = Number(chatItem.dataset.userId);
            loadMessages(selectedUserId);
            appendCompanionInfInChat(`${user.firstname} ${user.lastname}`);
        });
        chatItem.appendChild(userInfo);
        chatItems.appendChild(chatItem);

        getLastMessageWithUser(user.id)

    });
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
        console.log("vaaaaaa");
        lastMessageElement.innerHTML = "";
    }
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

function loadMessages(userId) {
    const payload = JSON.stringify({command: "get_messages", user_id: userId});
    socket.send(payload);
}

function getLastMessageWithUser(userId) {
    const payload = JSON.stringify({command: "get_last_message", user_id: userId});
    socket.send(payload);
}

function setActiveChat(chatItem) {
    const currentActive = document.querySelector('.chat-item.active-chat');
    if (currentActive) {
        currentActive.classList.remove('active-chat');
    }
    chatItem.classList.add('active-chat');
}


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
    document.getElementById('login').style.display = 'flex';
    document.getElementById('registration').style.display = 'none';
    document.getElementById('content').style.display = 'none'
    document.getElementById('header').style.display = 'none'
    document.getElementById('companionInfInChat').style.display = 'none';
    document.getElementById('messages').style.display = 'none';
    document.getElementById('messageInputContainer').style.display = 'none';
    document.getElementById('connectButton').style.display = 'block';
}
