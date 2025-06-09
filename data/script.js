async function callToggleSwitch(ch) {
    try {
        const response = await fetch('/ToggleSwitch?ch=' + ch);
        const data = await response.json();
        console.log(data);
    } catch (error) {
        console.error('Error:', error);
    }
    location.reload();
}


document.addEventListener("DOMContentLoaded", function() {
    fetch('channelNames.json')
        .then(response => response.json())
        .then(data => {
            insertChannelNames(data);
        })
        .catch(error => console.error('Error fetching channel names:', error));
});

function insertChannelNames(channelNames) {
    const channelElements = document.querySelectorAll('.channels .listItemName');

    channelElements.forEach((element, index) => {
        if (channelNames[index]) {
            element.textContent = index+1 + ". " + channelNames[index];
        }
    });
}