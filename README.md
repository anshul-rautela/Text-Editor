# Text Editor Implemented in C Programming Language

This project is a **simple GUI-based text editor** built using the **C programming language** and **GTK 3** for the graphical interface. It is designed to demonstrate low-level text handling techniques, including a custom piece table data structure and undo/redo functionality.

---

## ✨ Features
- **File Handling** (open and save functionality)
- **Undo / Redo System**

---

## 🛠️ Installation & Run Instructions

Follow these steps to install and run the editor:

 1. Clone the repository
```bash
git clone https://github.com/anshul-rautela/Text-Editor.git
cd Text-Editor
```

 2. Update package list
```bash
sudo apt update
```

 3. Install GTK 3 development libraries
```bash
sudo apt install libgtk-3-dev
```

 4. Compile the code
```bash
gcc main.c -o editor `pkg-config --cflags --libs gtk+-3.0`
```

 5. Run the text editor
```bash
./editor
```
