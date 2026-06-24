const ocrForm = document.getElementById("ocr-form");
const imageInput = document.getElementById("image-input");
const previewBox = document.getElementById("preview-box");
const statusBox = document.getElementById("status-box");
const ocrResult = document.getElementById("ocr-result");
const fileInfo = document.getElementById("file-info");

let selectedFile = null;

if (ocrForm && imageInput && previewBox && statusBox && ocrResult && fileInfo) {
  console.debug("[OCR] Interface initialized");

  imageInput.addEventListener("change", () => {
    selectedFile = imageInput.files[0] ?? null;

    if (!selectedFile) {
      resetPreview();
      return;
    }

    const validationError = validateImageFile(selectedFile);

    if (validationError) {
      console.warn("[OCR] File validation failed:", validationError);
      setStatus(validationError, "error");
      resetPreview(false);
      imageInput.value = "";
      selectedFile = null;
      return;
    }

    console.debug("[OCR] File selected:", selectedFile.name);
    showImagePreview(selectedFile);
    showFileInfo(selectedFile);
    setStatus("File loaded correctly. Ready to run OCR.", "success");
  });

  ocrForm.addEventListener("submit", async (event) => {
    event.preventDefault();

    if (!selectedFile) {
      setStatus("Please select an image first.", "error");
      return;
    }

    setStatus("Processing image...", "processing");
    ocrResult.value = "";

    try {
      const result = await simulateOcrRequest(selectedFile);

      ocrResult.value = result.text;

      saveResultToHistory({
        fileName: selectedFile.name,
        fileSize: selectedFile.size,
        fileType: selectedFile.type,
        resultText: result.text,
        createdAt: new Date().toISOString()
      });

      console.debug("[OCR] Processing completed successfully");
      setStatus("OCR finished successfully.", "success");
    } catch (error) {
      console.error("[OCR] Processing failed:", error);
      setStatus("OCR failed: " + error.message, "error");
    }
  });
}

function validateImageFile(file) {
  const allowedTypes = ["image/png", "image/jpeg", "image/jpg"];
  const maxSizeInMB = 5;
  const maxSizeInBytes = maxSizeInMB * 1024 * 1024;

  if (!allowedTypes.includes(file.type)) {
    return "Invalid file type. Please choose PNG or JPG image.";
  }

  if (file.size > maxSizeInBytes) {
    return "File is too large. Maximum size is 5 MB.";
  }

  return null;
}

function showImagePreview(file) {
  const reader = new FileReader();

  reader.addEventListener("load", () => {
    previewBox.replaceChildren();

    const image = document.createElement("img");
    image.src = reader.result;
    image.alt = "Selected image preview";

    previewBox.appendChild(image);
  });

  reader.addEventListener("error", () => {
    setStatus("Image preview could not be loaded.", "error");
  });

  reader.readAsDataURL(file);
}

function showFileInfo(file) {
  const details = [
    ["Name:", file.name],
    ["Type:", file.type],
    ["Size:", `${(file.size / 1024).toFixed(2)} KB`]
  ];

  const fragment = document.createDocumentFragment();

  details.forEach(([label, value]) => {
    const item = document.createElement("li");
    const strong = document.createElement("strong");

    strong.textContent = label;
    item.append(strong, ` ${value}`);
    fragment.appendChild(item);
  });

  fileInfo.replaceChildren(fragment);
}

function resetPreview(resetStatus = true) {
  const message = document.createElement("p");
  const listItem = document.createElement("li");

  message.textContent = "No image selected";
  listItem.textContent = "No file loaded";

  previewBox.replaceChildren(message);
  fileInfo.replaceChildren(listItem);
  ocrResult.value = "";

  if (resetStatus) {
    setStatus("Status: waiting for file", "");
  }
}

function setStatus(message, type) {
  statusBox.textContent = message;
  statusBox.classList.remove("processing", "success", "error");

  if (type) {
    statusBox.classList.add(type);
  }
}

function simulateOcrRequest(file) {
  return new Promise((resolve, reject) => {
    window.setTimeout(() => {
      if (file.name.toLowerCase().includes("error")) {
        reject(new Error("Simulated server error"));
        return;
      }

      resolve({
        text:
`Simulated OCR result

File: ${file.name}

Detected text:
Lorem ipsum dolor sit amet.
This is a test result from simulated OCR engine.

Confidence: 92%
Engine: SimulatedOCR v1.0`
      });
    }, 1500);
  });
}

function saveResultToHistory(result) {
  try {
    const history = JSON.parse(localStorage.getItem("ocrHistory")) || [];
    history.push(result);
    localStorage.setItem("ocrHistory", JSON.stringify(history));
  } catch (error) {
    console.error("[OCR] History could not be saved:", error);
  }
}
