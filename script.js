let socket;
let userId;

document.getElementById('connectButton').addEventListener('click', () => {
    // Connecting to the WebSocket server
    socket = new WebSocket('ws://127.0.0.1:9001/');

    // Handling connection open event
    socket.onopen = function(event) {
        console.log("Connected to server");
        appendMessage("Connected to server");
        document.getElementById('connectionStatus').innerText = "Connected";
        document.getElementById('connectButton').style.display = 'none';
        document.getElementById('auth').style.display = 'block';

        // Request user list immediately after connecting
        const payload = JSON.stringify({command: "user_list_db"});
        socket.send(payload);
    };

    // Handling incoming messages
    socket.onmessage = function(event) {
        console.log(">server>", event.data);

        if (typeof event.data === 'string') {
            try {
                const parsedData = JSON.parse(event.data);

                if (parsedData.command === "registered" && parsedData.user_id !== undefined) {
                    document.getElementById('connectionStatus').innerText = "Registered (User ID: " + parsedData.user_id + ")";
                    document.getElementById('registration').style.display = 'none';
                    document.getElementById('chat').style.display = 'flex';
                } else if (parsedData.command === "logged_in" && parsedData.user_id !== undefined) {
                    document.getElementById('connectionStatus').innerText = "Logged in as " + parsedData.name + " (User ID: " + parsedData.user_id + ")";
                    document.getElementById('login').style.display = 'none';
                    document.getElementById('chat').style.display = 'flex';
                    parsedData.messages.forEach(msg => appendMessage(msg.message));
                } else if (parsedData.command === "login_failed") {
                    appendMessage("Login failed: " + parsedData.message);
                } else if (parsedData.command === "logged_out") {
                    document.getElementById('connectionStatus').innerText = "Logged out";
                    document.getElementById('chat').style.display = 'none';
                    document.getElementById('auth').style.display = 'block';
                } else if (Array.isArray(parsedData)) {
                    const userSelect = document.getElementById('userSelect');
                    userSelect.innerHTML = '<option value="">Select user (optional)</option>';
                    parsedData.forEach(user => {
                        const option = document.createElement('option');
                        option.value = user.id;
                        option.text = `${user.name} (ID: ${user.id})`;
                        userSelect.appendChild(option);
                    });
                } else if (parsedData.command === "public_msg" || parsedData.command === "private_msg") {
                    appendMessage(parsedData.text);
                }
            } catch (e) {
                appendMessage(event.data);
            }
        } else {
            appendMessage("Received non-text message");
        }
    };

    // Handling connection close event
    socket.onclose = function(event) {
        console.log("Disconnected from server");
        appendMessage("Disconnected from server");
        document.getElementById('connectionStatus').innerText = "Disconnected";
    };

    // Handling errors
    socket.onerror = function(error) {
        console.error("WebSocket error: ", error);
        appendMessage("WebSocket error: " + error.message);
        document.getElementById('connectionStatus').innerText = "Error";
    };
});

document.getElementById('registerButton').addEventListener('click', () => {
    document.getElementById('auth').style.display = 'none';
    document.getElementById('registration').style.display = 'block';
});

document.getElementById('registerSubmitButton').addEventListener('click', () => {
    const name = document.getElementById('regNameInput').value;
    const password = document.getElementById('regPasswordInput').value;
    if (name.trim() !== "" && password.trim() !== "") {
        const payload = JSON.stringify({command: "register", name: name, password: password});
        socket.send(payload);
    }
});

document.getElementById('loginButton').addEventListener('click', () => {
    document.getElementById('auth').style.display = 'none';
    document.getElementById('login').style.display = 'block';
});

document.getElementById('loginSubmitButton').addEventListener('click', () => {
    const name = document.getElementById('loginNameInput').value;
    const password = document.getElementById('loginPasswordInput').value;
    if (name.trim() !== "" && password.trim() !== "") {
        const payload = JSON.stringify({command: "login", name: name, password: password});
        socket.send(payload);
    }
});

document.getElementById('sendMessageButton').addEventListener('click', () => {
    const message = document.getElementById('messageInput').value;
    const toWhom = document.getElementById('userSelect').value;

    if (message.trim() !== "" && toWhom !== "") {
        const payload = JSON.stringify({command: "private_msg", text: message, user_to: parseInt(toWhom)});
        socket.send(payload);
        document.getElementById('messageInput').value = ""; // Clear input field after sending
    } else if (message.trim() !== "") {
        const payload = JSON.stringify({command: "public_msg", text: message});
        socket.send(payload);
        document.getElementById('messageInput').value = ""; // Clear input field after sending
    }
});

document.getElementById('logoutButton').addEventListener('click', () => {
    const payload = JSON.stringify({command: "logout"});
    socket.send(payload);
});

// Function to append messages to the page
function appendMessage(message) {
    const messageDiv = document.createElement('div');
    messageDiv.textContent = message;
    messageDiv.className = "message";
    document.getElementById('messages').appendChild(messageDiv);
}
