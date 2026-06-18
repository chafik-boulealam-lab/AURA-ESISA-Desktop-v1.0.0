const API_BASE = "http://localhost:8080/api";
let currentEmail = "";

const output = document.getElementById("output");
const tasksEl = document.getElementById("tasks");
const notesEl = document.getElementById("notes");
const aiReplyEl = document.getElementById("aiReply");

function show(obj) {
  output.textContent = JSON.stringify(obj, null, 2);
}

async function api(path, method = "GET", body) {
  const options = { method, headers: { "Content-Type": "application/json" } };
  if (body) options.body = JSON.stringify(body);
  const res = await fetch(`${API_BASE}${path}`, options);
  const data = await res.json();
  if (!res.ok || data.ok === false) {
    throw new Error(data.error || `HTTP ${res.status}`);
  }
  return data;
}

function getAuthInput() {
  const email = document.getElementById("email").value.trim();
  const password = document.getElementById("password").value;
  return { email, password };
}

document.getElementById("btnRegister").addEventListener("click", async () => {
  try {
    const { email, password } = getAuthInput();
    const data = await api("/register", "POST", { email, password });
    currentEmail = email;
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});

document.getElementById("btnLogin").addEventListener("click", async () => {
  try {
    const { email, password } = getAuthInput();
    const data = await api("/login", "POST", { email, password });
    currentEmail = email;
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});

document.getElementById("btnAddTask").addEventListener("click", async () => {
  try {
    const title = document.getElementById("taskTitle").value.trim();
    const data = await api("/tasks", "POST", { email: currentEmail, title });
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});

document.getElementById("btnLoadTasks").addEventListener("click", async () => {
  try {
    const data = await api(`/tasks?email=${encodeURIComponent(currentEmail)}`);
    tasksEl.innerHTML = "";
    data.tasks.forEach((t) => {
      const li = document.createElement("li");
      li.textContent = `#${t.id} ${t.title} (done=${t.done})`;
      tasksEl.appendChild(li);
    });
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});

document.getElementById("btnAddNote").addEventListener("click", async () => {
  try {
    const content = document.getElementById("noteContent").value.trim();
    const data = await api("/notes", "POST", { email: currentEmail, content });
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});

document.getElementById("btnLoadNotes").addEventListener("click", async () => {
  try {
    const data = await api(`/notes?email=${encodeURIComponent(currentEmail)}`);
    notesEl.innerHTML = "";
    data.notes.forEach((n) => {
      const li = document.createElement("li");
      li.textContent = `#${n.id} ${n.content}`;
      notesEl.appendChild(li);
    });
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});

document.getElementById("btnAskAI").addEventListener("click", async () => {
  try {
    const prompt = document.getElementById("prompt").value.trim();
    const data = await api("/ai", "POST", { prompt });
    aiReplyEl.textContent = data.reply;
    show(data);
  } catch (e) {
    show({ ok: false, error: e.message });
  }
});
