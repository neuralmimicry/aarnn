resource "kubernetes_namespace" "aarnn" {
  metadata {
    name = "aarnn"
  }
}

resource "kubernetes_pod" "multi_container_pod" {
  metadata {
    name      = "multi-container-pod"
    namespace = kubernetes_namespace.aarnn.metadata[0].name
    labels = {
      app = "multi-container-pod"
    }
  }

  spec {
    container {
      name  = "aarnn-webui"
      image = "europe-west1-docker.pkg.dev/${var.project_id}/aarnn-webui-repo/aarnn-webui-app:v1"
      ports {
        container_port = 8080
      }
      env {
        name  = "PORT"
        value = "8080"
      }
      resources {
        limits {
          cpu    = "250m"
          memory = "512Mi"
        }
        requests {
          cpu    = "100m"
          memory = "256Mi"
        }
      }
    }

    container {
      name  = "aarnn-audioprompt"
      image = "europe-west1-docker.pkg.dev/${var.project_id}/aarnn-audioprompt-repo/aarnn-audioprompt-app:v1"
      ports {
        container_port = 8081
      }
      env {
        name  = "PORT"
        value = "8081"
      }
      resources {
        limits {
          cpu    = "250m"
          memory = "512Mi"
        }
        requests {
          cpu    = "100m"
          memory = "256Mi"
        }
      }
    }

    container {
      name  = "aarnn-health"
      image = "europe-west1-docker.pkg.dev/${var.project_id}/aarnn-health-repo/aarnn-health-app:v1"
      ports {
        container_port = 8082
      }
      env {
        name  = "PORT"
        value = "8082"
      }
      resources {
        limits {
          cpu    = "250m"
          memory = "512Mi"
        }
        requests {
          cpu    = "100m"
          memory = "256Mi"
        }
      }
    }

    container {
      name  = "aarnn-zeropercent"
      image = "europe-west1-docker.pkg.dev/${var.project_id}/aarnn-zeropercent-repo/aarnn-zeropercent-app:v1"
      ports {
        container_port = 8083
      }
      env {
        name  = "PORT"
        value = "8083"
      }
      resources {
        limits {
          cpu    = "250m"
          memory = "512Mi"
        }
        requests {
          cpu    = "100m"
          memory = "256Mi"
        }
      }
    }

    volumes {
      name = "shared-data"
      empty_dir {}
    }
  }
}

resource "kubernetes_service" "webui_service" {
  metadata {
    name      = "aarnn-webui-service"
    namespace = kubernetes_namespace.aarnn.metadata[0].name
  }

  spec {
    selector = {
      app = kubernetes_pod.multi_container_pod.metadata[0].labels["app"]
    }

    port {
      port        = 80
      target_port = 8080
    }

    type = "LoadBalancer"
  }
}
