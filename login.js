function handleLoggedIn(parsedData) {
    document.getElementById('connectionStatus').innerText = "Logged in as " + parsedData.name + " (User ID: " + parsedData.user_id + ")";
    document.getElementById('loginNameInput').style.marginBottom = "20px";
    document.getElementById('loginPasswordInput').style.marginBottom = "20px";
    document.getElementById('nameFail').style.display = "none";
    document.getElementById('passwordFail').style.display = "none";
    curUserID = parsedData.user_id;
    showChatElements();
}

function handleLoginFail(parsedData) {
    if (parsedData.fail === "name") {
        document.getElementById('nameFail').style.display = "block";
        document.getElementById('loginNameInput').style.marginBottom = "0";
        document.getElementById('loginPasswordInput').style.marginBottom = "20px";
        document.getElementById('passwordFail').style.display = "none";
    } else if (parsedData.fail === "password"){
        document.getElementById('nameFail').style.display = "none";
        document.getElementById('loginNameInput').style.marginBottom = "20px";
        document.getElementById('passwordFail').style.display = "block";
        document.getElementById('loginPasswordInput').style.marginBottom = "0";

    }
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