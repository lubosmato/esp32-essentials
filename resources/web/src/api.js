async function get(key) {
  try {
    const response = await fetch(`/config/${key}`, { method: "GET" });
    const value = await response.text();
    if (response.status !== 200) throw Error("Given value does not exist");
    return value;
  } catch (e) {
    throw Error(`Couldn't get value in API: ${e}`);
  }
}

async function set(key, value) {
  try {
    const response = await fetch(`/config/${key}`, {
      method: "POST",
      body: value
    });
    if (response.status !== 200) throw Error("Given value does not exist");
    const text = await response.text();
    if (text != value) {
      throw Error("Device couldn't store given value");
    }
  } catch (e) {
    throw Error(`Couldn't set value in API: ${e}`);
  }
}

export default { get, set };
