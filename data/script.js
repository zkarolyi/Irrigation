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