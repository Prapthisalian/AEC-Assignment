const API_BASE = 'http://localhost:18080';

// --- Auth Helpers ---
function setSession(userId) {
    localStorage.setItem('user_id', userId);
}

function getSession() {
    return localStorage.getItem('user_id');
}

function clearSession() {
    localStorage.removeItem('user_id');
    window.location.href = 'index.html';
}

function requireAuth() {
    if (!getSession()) {
        window.location.href = 'login.html';
    }
}

// --- API Calls ---

async function signup(name, email, password) {
    try {
        const response = await fetch(`${API_BASE}/signup`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name, email, password })
        });

        if (response.ok) {
            return { success: true };
        } else {
            const text = await response.text();
            return { success: false, error: text };
        }
    } catch (e) {
        return { success: false, error: e.message };
    }
}

async function login(email, password) {
    try {
        const response = await fetch(`${API_BASE}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ email, password })
        });

        if (response.ok) {
            const data = await response.json();
            setSession(data.user_id);
            localStorage.setItem('user_name', data.name);
            localStorage.setItem('user_email', data.email);
            return { success: true };
        } else {
            const text = await response.text();
            return { success: false, error: text };
        }
    } catch (e) {
        return { success: false, error: e.message };
    }
}

async function addEntry(date, text) {
    const userId = getSession();
    if (!userId) return { success: false, error: "Not logged in" };

    try {
        const response = await fetch(`${API_BASE}/diary`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ user_id: parseInt(userId), date, text })
        });

        if (response.ok) {
            const data = await response.json();
            return { success: true, analysis: data.analysis };
        } else {
            return { success: false, error: await response.text() };
        }
    } catch (e) {
        return { success: false, error: e.message };
    }
}

async function getHistory() {
    const userId = getSession();
    if (!userId) return [];

    try {
        const response = await fetch(`${API_BASE}/diary/history?user_id=${userId}`);
        if (response.ok) {
            return await response.json();
        }
    } catch (e) {
        console.error(e);
    }
    return [];
}

async function getUserStats() {
    const userId = getSession();
    if (!userId) return { streak: 0, total_entries: 0 };

    try {
        const response = await fetch(`${API_BASE}/user/stats?user_id=${userId}`);
        if (response.ok) {
            return await response.json();
        }
    } catch (e) {
        console.error(e);
    }
    return { streak: 0, total_entries: 0 };
}

// --- Theme Helper ---
function applyTheme() {
    const theme = localStorage.getItem('theme');
    const toggle = document.getElementById('dark-mode-toggle');

    // Apply class
    if (theme === 'dark') {
        document.body.classList.add('dark-mode');
        if (toggle) toggle.checked = true;
    } else {
        document.body.classList.remove('dark-mode');
        if (toggle) toggle.checked = false;
    }
}

function toggleTheme(e) {
    if (e.target.checked) {
        document.body.classList.add('dark-mode');
        localStorage.setItem('theme', 'dark');
    } else {
        document.body.classList.remove('dark-mode');
        localStorage.setItem('theme', 'light');
    }
}

// Init Theme Controls
function initTheme() {
    applyTheme();

    // If on profile page (or any page with the toggle), attach listener
    const toggle = document.getElementById('dark-mode-toggle');
    if (toggle) {
        toggle.addEventListener('change', toggleTheme);
    }
}

// Apply on load
document.addEventListener('DOMContentLoaded', initTheme);

// --- Analytics Helpers ---
function calculateStreak(sortedHistory) {
    if (!sortedHistory || sortedHistory.length === 0) return 0;

    // Ensure sorted by date ascending
    const uniqueDates = [...new Set(sortedHistory.map(h => h.date))].sort().reverse();
    // Now descending: today, yesterday...

    if (uniqueDates.length === 0) return 0;

    const today = new Date().toISOString().split('T')[0];
    const yesterday = new Date(Date.now() - 86400000).toISOString().split('T')[0];

    let streak = 0;

    // If the latest entry is not today AND not yesterday, streak is 0
    if (uniqueDates[0] !== today && uniqueDates[0] !== yesterday) {
        return 0;
    }

    let checkDate = new Date();
    // If latest is today, start checking from today. If latest is yesterday, start from yesterday.
    if (uniqueDates[0] !== today) {
        checkDate.setDate(checkDate.getDate() - 1); // Start from yesterday
    }

    for (let i = 0; i < uniqueDates.length; i++) {
        const target = checkDate.toISOString().split('T')[0];
        if (uniqueDates[i] === target) {
            streak++;
            checkDate.setDate(checkDate.getDate() - 1); // Move back one day
        } else {
            break;
        }
    }

    return streak;
}



