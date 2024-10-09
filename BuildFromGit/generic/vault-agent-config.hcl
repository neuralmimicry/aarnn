exit_after_auth = false
pid_file = "/var/run/vault/vault-agent.pid"

auto_auth {
  method "kubernetes" {
    mount_path = "auth/kubernetes"
    config = {
      role = "postgres-role"
    }
  }

  sink "file" {
    config = {
      path = "/var/run/secrets/vault-token"
    }
  }
}

template {
  source      = "/etc/vault-agent-config/template-config.ctmpl"
  destination = "/etc/secrets/postgres-credentials"
}
