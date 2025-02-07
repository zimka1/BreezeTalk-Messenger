function handleRegistered(parsedData) {
    document.getElementById('connectionStatus').innerText = "Logged in as " + parsedData.name + " (User ID: " + parsedData.user_id + ")";
    curUserID = parsedData.user_id;
    showChatElements();
}

document.getElementById('registerSubmitButton').addEventListener('click', () => {
    const firstname = document.getElementById('firstNameInput').value;
    const lastname = document.getElementById('lastNameInput').value;
    const nickname = document.getElementById('regNameInput').value;
    const password = document.getElementById('regPasswordInput').value;

    document.getElementById('regNameInput').value = "";
    document.getElementById('regPasswordInput').value = "";
    document.getElementById('firstNameInput').value = "";
    document.getElementById('lastNameInput').value = "";

    if (nickname.trim() !== "" && password.trim() !== "") {
        const payload = JSON.stringify({command: "register", firstname: firstname, lastname: lastname, nickname: nickname, password: password});
        socket.send(payload);
    }
});