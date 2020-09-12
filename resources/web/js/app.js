import "../scss/app.scss"

async function load() {
  const response = await fetch("/settings", { method: "GET" })
  const settings = await response.json()
  const { deviceName, version } = settings
  const fields = Object.fromEntries(
    Object.entries(settings)
      .filter(([label]) => !(label === "deviceName" || label === "version"))
  )

  document.querySelectorAll(".device-name").forEach(el => el.innerText = deviceName)
  document.querySelectorAll(".version").forEach(el => el.innerText = version)

  const formFields = document.querySelector(".fields")
  for (const [labelText, value] of Object.entries(fields)) {
    const field = document.querySelector(".field-template").cloneNode(true)
    field.classList.remove("field-template")

    const label = field.querySelector("label")
    label.innerText = labelText
    label.setAttribute("for", labelText)

    const input = field.querySelector("input")
    input.name = labelText
    input.value = value
    input.setAttribute("id", labelText)

    formFields.append(field)
  }

  document.getElementById("settings-form").onsubmit = async (e) => {
    e.preventDefault()

    const fields = Array.from(document.querySelectorAll("#settings-form *[name]"))
    const entries = fields.map(el => [el.getAttribute("name"), el.value])
    const data = Object.fromEntries(entries)

    console.log(JSON.stringify(data))

    await fetch("/settings", {
      method: "POST",
      body: JSON.stringify(data)
    })
    window.location.reload()

    return false
  }
  
}

window.onload = () => load()
