resource "google_compute_firewall" "allow_webui_external_access" {
  name    = "allow-webui-external-access"
  network = "default"

  allow {
    protocol = "tcp"
    ports    = ["80"]
  }

  source_ranges = ["0.0.0.0/0"]
  target_tags   = ["aarnn-webui"]
  description   = "Allow external access to web UI on port 80"
}
