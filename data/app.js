let ws = null;
let reconnectTimer = null;
let autoSaveEnabled = true;

document.addEventListener('DOMContentLoaded', () => {
    initTabs();
    initButtons();
    connectWebSocket();
});

function initTabs() {
    const tabBtns = document.querySelectorAll('.tab-btn');
    tabBtns.forEach(btn => {
        btn.addEventListener('click', () => {
            tabBtns.forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            btn.classList.add('active');
            document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
        });
    });
}

function initButtons() {
    document.getElementById('btnTare').addEventListener('click', () => {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send('tare');
        }
    });

    document.getElementById('btnClearHistory').addEventListener('click', () => {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send('clear_history');
            document.getElementById('historyTableBody').innerHTML = '<tr><td colspan="3">Nenhum registro</td></tr>';
            document.getElementById('historyCount').textContent = '0';
        }
    });

    document.getElementById('btnToggleAutoSave').addEventListener('click', () => {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send('toggle_autosave');
        }
    });

    document.getElementById('btnCalibrate').addEventListener('click', () => {
        const weight = document.getElementById('knownWeight').value;
        if (weight && parseFloat(weight) > 0) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send('calibrate:' + weight);
            }
        }
    });

    document.getElementById('btnSetFactor').addEventListener('click', () => {
        const factor = document.getElementById('manualFactor').value;
        if (factor && parseFloat(factor) > 0) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send('factor:' + factor);
            }
        }
    });

    document.getElementById('btnResetCalibration').addEventListener('click', () => {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send('reset');
        }
    });
}

function connectWebSocket() {
    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = protocol + '//' + location.host + ':81/';

    ws = new WebSocket(wsUrl);

    ws.onopen = () => {
        document.getElementById('connectionStatus').className = 'status-dot connected';
        document.getElementById('wifiStatus').textContent = 'Conectado';
        if (reconnectTimer) {
            clearTimeout(reconnectTimer);
            reconnectTimer = null;
        }
    };

    ws.onclose = () => {
        document.getElementById('connectionStatus').className = 'status-dot disconnected';
        document.getElementById('wifiStatus').textContent = 'Desconectado';
        reconnectTimer = setTimeout(connectWebSocket, 3000);
    };

    ws.onerror = () => {
        ws.close();
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            updateUI(data);
        } catch (e) {
            console.error('Invalid JSON:', event.data);
        }
    };
}

function updateUI(data) {
    // Weight display
    document.getElementById('currentWeight').textContent = data.weight.toFixed(2);
    document.getElementById('weightStatus').textContent = 'Peso: ' + data.weight.toFixed(2) + ' kg';

    // Tare progress
    const tareProgress = document.getElementById('tareProgress');
    const tareProgressBar = document.getElementById('tareProgressBar');
    const tareProgressText = document.getElementById('tareProgressText');

    if (data.tareInProgress) {
        tareProgress.classList.remove('hidden');
        const pct = (data.tareSamples / data.tareTotal) * 100;
        tareProgressBar.style.width = pct + '%';
        tareProgressText.textContent = 'Tarando... ' + data.tareSamples + '/' + data.tareTotal;
    } else {
        tareProgress.classList.add('hidden');
    }

    // Info cards
    document.getElementById('displayFactor').textContent = data.factor.toFixed(2);
    document.getElementById('displayOffset').textContent = data.offset;

    // Battery status
    if (data.batteryPercent !== undefined) {
        const pct = data.batteryPercent;
        const volt = data.batteryVoltage ? data.batteryVoltage.toFixed(2) : '--';
        document.getElementById('displayBattery').textContent = pct + '% (' + volt + 'V)';
        document.getElementById('batteryStatus').textContent = '🔋 ' + pct + '%';
        
        // Color based on level
        const batEl = document.getElementById('batteryStatus');
        if (pct <= 20) batEl.style.color = '#ff1744';
        else if (pct <= 50) batEl.style.color = '#ffab00';
        else batEl.style.color = '#00c853';
    }

    // WiFi status
    let wifiText = '';
    if (data.wifiConnected) {
        wifiText = 'STA: ' + data.staIP;
    } else if (data.apActive) {
        wifiText = 'AP: ' + data.apIP;
    } else {
        wifiText = 'Desconectado';
    }
    document.getElementById('displayWifi').textContent = wifiText;
    document.getElementById('cfgStaIP').textContent = data.wifiConnected ? data.staIP : '--';
    document.getElementById('cfgApIP').textContent = data.apIP;

    // Auto-save button
    autoSaveEnabled = data.autoSave;
    document.getElementById('btnToggleAutoSave').textContent = 'Auto-Save: ' + (autoSaveEnabled ? 'ON' : 'OFF');

    // History count
    document.getElementById('historyCount').textContent = data.historyCount;

    // History table
    if (data.history && data.history.length > 0) {
        const tbody = document.getElementById('historyTableBody');
        tbody.innerHTML = '';
        data.history.forEach((entry, index) => {
            const row = document.createElement('tr');
            row.innerHTML = '<td>' + (index + 1) + '</td><td>' + entry.weight.toFixed(2) + '</td><td>' + entry.time + '</td>';
            tbody.appendChild(row);
        });
    }
}
