import "../scss/app.scss"

async function load() {
  const response = await fetch("/settings", { method: "GET" })
  const settings = await response.json()
  const { deviceName, version, ssid, pass } = settings

  document.querySelectorAll(".device-name").forEach(el => el.innerText = deviceName)
  document.querySelectorAll(".version").forEach(el => el.innerText = version)
  document.getElementById("ssid").value = ssid
  document.getElementById("pass").value = pass

  document.getElementById("settings-form").onsubmit = async (e) => {
    e.preventDefault()

    const fields = Array.from(document.querySelectorAll("#settings-form *[name]"))
    const entries = fields.map(el => [el.getAttribute("name"), el.value])
    const data = Object.fromEntries(entries)

    await fetch("/settings", {
      method: "POST",
      body: JSON.stringify(data)
    })
    window.location.reload()

    return false
  }
  
}

window.onload = () => load()
