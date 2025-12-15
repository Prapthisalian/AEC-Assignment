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

// --- Theme Helper ---
function applyTheme() {
    // FORCE RESET: Overwrite any stuck 'dark' preference to 'light'
    let theme = localStorage.getItem('theme');

    // Auto-fix for stuck users
    if (theme === 'dark') {
        console.log("Resetting theme to light");
        localStorage.setItem('theme', 'light');
        theme = 'light';
    }

    if (theme === 'dark') {
        document.body.classList.add('dark-mode');
    } else {
        document.body.classList.remove('dark-mode');
    }
}

// Apply on load
document.addEventListener('DOMContentLoaded', applyTheme);

