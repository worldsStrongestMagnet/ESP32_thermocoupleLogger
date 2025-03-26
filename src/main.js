// DOM selectors 
const logIntervalObj = document.getElementById('interval');
const logDurationObj = document.getElementById('loggingDuration');
const currentTempDispObj = document.getElementById('currentTempDisp');

async function fetchLatestTemp() {
    try {
        // fetch latest temp from sensor
        const response = await fetch('/latest');
        const temp = await response.text();

        // insert current temp into html
        currentTempDispObj.innerText = temp + "°C";

    } catch (error) {
        console.error('error fetching temp: ', error);
    }
}

async function setLogParams() {
    try {
        // prevent form from clearing - annoying behavior
        event.preventDefault();

        // get values from form
        let logInterval = logIntervalObj.value;
        let logDuration = logDurationObj.value;

        const data = "inverval=" + encodeURIComponent(logInterval) + "&duration=" + encodeURIComponent(logDuration);

        // add code to push params to server
        fetch('/update', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded'
            },
            body: data
        })
        .then(response => response.text())
        .then(text => alert("Server response: " + text))
        .catch(error => console.error('Error: ', error));

    } catch (error) {
        console.error('error updating params: ', error)
    }
}

function limitLogLen(interval, durration) {
    let totalDatapoints = durration / interval;

    if (totalDatapoints > 10000) {
        return interval * 10000;
    } else {
        return durration;
    }
}