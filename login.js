function handleLoggedIn(parsedData) {
    document.getElementById('connectionStatus').innerText = "Logged in as " + parsedData.name + " (User ID: " + parsedData.user_id + ")";
    curUserID = parsedData.user_id;
    showChatElements();
}

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