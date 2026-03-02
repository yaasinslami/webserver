let fileToDelete = null;
let userContext = null;

// --- Toast System ---
function showToast(message, type = 'success') {
    let container = document.querySelector('.toast-container');
    if (!container) {
        container = document.createElement('div');
        container.className = 'toast-container';
        document.body.appendChild(container);
    }

    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `<span>${type === 'success' ? '✅' : '❌'}</span><span>${message}</span>`;
    container.appendChild(toast);

    setTimeout(() => toast.classList.add('show'), 10);
    setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// --- Modal Controls ---
function openDeleteModal(filename, username) {
    fileToDelete = filename;
    userContext = username;
    
    console.log("Setting Modal for:", filename, "User:", username);
    document.getElementById('modalFileName').innerText = filename;
    document.getElementById('deleteModal').classList.add('active');
}

function closeModal() {
    document.getElementById('deleteModal').classList.remove('active');
}

// Close modal if user clicks outside the content box
window.onclick = function(event) {
    const modal = document.getElementById('deleteModal');
    if (event.target == modal) {
        closeModal();
    }
}

// --- The Actual Deletion ---
document.getElementById('confirmDeleteBtn').addEventListener('click', async () => {
    // Check local variables to ensure they aren't null
    if (!fileToDelete || !userContext) {
        console.error("Missing data!", fileToDelete, userContext);
        return;
    }

    const currentFile = fileToDelete;
    const currentUser = userContext; 

    closeModal();

    try {
        const url = `/cgi-bin/delete_handler.py?filename=${encodeURIComponent(currentFile)}&user=${encodeURIComponent(currentUser)}`;
        console.log("Fetching:", url);

        const response = await fetch(url);

        if (response.ok) {
            showToast('Email deleted successfully', 'success');
            const card = document.getElementById(`card-${currentFile}`);
            if (card) {
                card.style.opacity = '0';
                setTimeout(() => card.remove(), 400);
            }
        } else {
            showToast('Delete failed: Permission denied', 'error');
        }
    } catch (err) {
        showToast('Server error', 'error');
    }
});


let addContextUser = null;

function openAddModal(username) {
    addContextUser = username;
    document.getElementById('addModal').classList.add('active');
}

function closeAddModal() {
    document.getElementById('addModal').classList.remove('active');
    document.getElementById('newFileName').value = '';
    document.getElementById('newFileContent').value = '';
}

async function handleAddEmail() {
    const sender = document.getElementById('newSender').value || "Unknown";
    const filename = document.getElementById('newFileName').value;
    const body = document.getElementById('newFileContent').value;

    if (!filename) { showToast('Subject is required', 'error'); return; }

    const formData = new FormData();
    formData.append('content', `${sender}|${body}`);
    formData.append('filename', filename);
    formData.append('user', addContextUser);

    try {
        const response = await fetch('/cgi-bin/add_handler.py', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            showToast('Email sent successfully!', 'success');
            closeAddModal();
            setTimeout(() => location.reload(), 1000);
        } else {
            showToast('Failed to send email', 'error');
            closeAddModal();
        }
    } catch (err) {
        showToast('Server error', 'error');
        closeAddModal();
    }
}
