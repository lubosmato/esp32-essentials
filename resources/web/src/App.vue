<template>
  <div>
    <div class="container">
      <div class="row">
        <div class="column">
          <form @submit.prevent="save">
            <h1>{{ title }} v{{ version }}</h1>
            <p class="lead">
              {{ title }} is offline. In order to continue, please configure
              WiFi network.
            </p>
            <div v-for="(value, key) in config" :key="key">
              <label :for="key">{{ value.label }}</label>
              <input
                :id="key"
                type="text"
                :placeholder="value.label"
                required
                v-model="value.value"
              />
              <small>{{ value.desc }}</small>
              <hr />
            </div>

            <button type="submit" class="button">
              Save & Reset
            </button>
          </form>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import api from "@/api.js";

export default {
  name: "Config",
  data() {
    return {
      title: document.title,
      config: {
        ssid: {
          value: "",
          label: "SSID",
          desc: "WiFi network name"
        },
        pass: {
          value: "",
          label: "Password",
          desc: "WiFi network password"
        }
      },
      version: ""
    };
  },
  methods: {
    save() {
      Object.entries(this.config).forEach(async ([key]) => {
        await api.set(key, this.config[key].value);
      });
      location.replace("/reset");
    }
  },
  async mounted() {
    Object.entries(this.config).forEach(async ([key]) => {
      this.config[key].value = await api.get(key);
    });
    this.version = await api.get("version");
  }
};
</script>

<style lang="sass">
@import "src/assets/app.sass"

body
  margin-top: 2em

input
  margin-bottom: 0

hr
  margin: 1em 0

@media (min-width: 40.0rem)
  .container
    width: 50vw

@media (min-width: 80.0rem)
  .container
    width: 40vw

@media (min-width: 120.0rem)
  .container
    width: 30vw
</style>
