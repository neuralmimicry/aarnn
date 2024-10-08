resource "kubernetes_pod" "api_service_pod" {
  metadata {
    name      = "api-service-pod"
    namespace = kubernetes_namespace.aarnn.metadata[0].name
    labels = {
      app = "api-service"
    }
  }

  spec {
    container {
      name  = "api-service"
      image = "gcr.io/${var.project_id}/api-service:v1"
      ports {
        container_port = 8080
      }
      env {
        name  = "GOOGLE_APPLICATION_CREDENTIALS"
        value = "/path/to/service_account.json"
      }
      volume_mounts {
        name      = "service-account"
        mount_path = "/path/to"
      }
    }

    volumes {
      name = "service-account"
      secret {
        secret_name = "service-account-key"
      }
    }
  }
}

resource "kubernetes_service" "api_service" {
  metadata {
    name      = "api-service"
    namespace = kubernetes_namespace.aarnn.metadata[0].name
  }

  spec {
    selector = {
      app = kubernetes_pod.api_service_pod.metadata[0].labels["app"]
    }

    port {
      port        = 80
      target_port = 8080
    }

    type = "LoadBalancer"
  }
}
