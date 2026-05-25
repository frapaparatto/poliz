#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <memory>
#include <stdexcept>

#include "../src/data/csv_client_repository.hpp"
#include "../src/data/csv_interaction_repository.hpp"
#include "../src/data/csv_policy_repository.hpp"
#include "../src/domain/client.hpp"
#include "../src/domain/client_data.hpp"
#include "../src/domain/interaction.hpp"
#include "../src/domain/policy.hpp"
#include "../src/service/client_service.hpp"

TEST_CASE("Client CSV round-trip preserves data", "[integration]") {
  auto tmp = std::filesystem::temp_directory_path() / "test_client_data.csv";

  SECTION("insert, save, find") {
    {
      insura::data::CsvClientRepository client_repo(tmp.string());

      insura::domain::Client client1(
          "John", "Carmack", "jcarmack@doom.edu", std::nullopt,
          std::string("Software Engineer"), std::string("Meta"), std::nullopt,
          std::nullopt, std::nullopt, insura::domain::Client::ClientStatus::NEW,
          std::string("Creator Of Doom"));

      insura::domain::Client client2(
          "Linus", "Torvalds", "linustorvalds@linuxfoundation.org",
          std::nullopt, std::string("Software Engineer"), std::nullopt,
          std::nullopt, std::nullopt, std::nullopt,
          insura::domain::Client::ClientStatus::NEW,
          std::string("Creator Of Linux"));

      client_repo.insertClient(client1);
      client_repo.insertClient(client2);

      client_repo.save();
    }

    insura::data::CsvClientRepository client_repo2(tmp.string());
    client_repo2.load();
    auto found = client_repo2.findAll();

    REQUIRE(found.size() == 2);
    REQUIRE(found[0].getFirstName() == "John");
  }

  SECTION(
      "insert one client with all fields, save, reload, verify all fields") {
    {
      insura::data::CsvClientRepository repo(tmp.string());

      insura::domain::Client client(
          "Ada", "Lovelace", "ada@babbage.io", std::string("+39 02 1234567"),
          std::string("Mathematician"), std::string("Analytical Engine Co."),
          std::string("10 Downing Street"), std::string("London"),
          std::string("SW1A 2AA"),
          insura::domain::Client::ClientStatus::CONTACTED,
          std::string("Pioneer of computing"));

      repo.insertClient(client);
      repo.save();
    }

    insura::data::CsvClientRepository repo2(tmp.string());
    repo2.load();
    auto all = repo2.findAll();

    REQUIRE(all.size() == 1);
    const auto& c = all[0];
    CHECK(c.getFirstName() == "Ada");
    CHECK(c.getLastName() == "Lovelace");
    CHECK(c.getEmail() == "ada@babbage.io");
    CHECK(c.getPhone().value() == "+39 02 1234567");
    CHECK(c.getJobTitle().value() == "Mathematician");
    CHECK(c.getCompany().value() == "Analytical Engine Co.");
    CHECK(c.getAddress().value() == "10 Downing Street");
    CHECK(c.getCity().value() == "London");
    CHECK(c.getPostalCode().value() == "SW1A 2AA");
    CHECK(c.getStatus() == insura::domain::Client::ClientStatus::CONTACTED);
    CHECK(c.getNotes().value() == "Pioneer of computing");
  }

  SECTION("insert two clients, save, reload, both are findable") {
    {
      insura::data::CsvClientRepository repo(tmp.string());

      insura::domain::Client client1(
          "Grace", "Hopper", "ghopper@navy.mil", std::nullopt,
          std::string("Rear Admiral"), std::nullopt, std::nullopt, std::nullopt,
          std::nullopt, insura::domain::Client::ClientStatus::NEW,
          std::nullopt);

      insura::domain::Client client2(
          "Alan", "Turing", "aturing@bletchley.uk", std::nullopt,
          std::string("Cryptanalyst"), std::nullopt, std::nullopt, std::nullopt,
          std::nullopt, insura::domain::Client::ClientStatus::NEW,
          std::nullopt);

      repo.insertClient(client1);
      repo.insertClient(client2);
      repo.save();
    }

    insura::data::CsvClientRepository repo2(tmp.string());
    repo2.load();

    REQUIRE(repo2.findAll().size() == 2);
    CHECK(repo2.findByEmail("ghopper@navy.mil").has_value());
    CHECK(repo2.findByEmail("aturing@bletchley.uk").has_value());
  }

  SECTION("edit, save, reload, changes are persistent") {
    std::string uuid;
    {
      insura::data::CsvClientRepository repo(tmp.string());

      insura::domain::Client client(
          "Dennis", "Ritchie", "dmr@bell-labs.com", std::nullopt,
          std::string("Computer Scientist"), std::nullopt, std::nullopt,
          std::nullopt, std::nullopt, insura::domain::Client::ClientStatus::NEW,
          std::nullopt);

      uuid = client.getUuid();
      repo.insertClient(client);
      repo.save();
    }

    {
      insura::data::CsvClientRepository repo(tmp.string());
      repo.load();

      auto result = repo.findByUuid(uuid);
      REQUIRE(result.has_value());

      auto updated = result.value();
      updated.setFirstName("Ken");
      updated.setCity("Murray Hill");
      updated.setStatus(insura::domain::Client::ClientStatus::CONTACTED);
      repo.updateClient(updated);
      repo.save();
    }

    insura::data::CsvClientRepository repo3(tmp.string());
    repo3.load();

    auto result = repo3.findByUuid(uuid);
    REQUIRE(result.has_value());
    CHECK(result->getFirstName() == "Ken");
    CHECK(result->getLastName() == "Ritchie");
    CHECK(result->getCity().value() == "Murray Hill");
    CHECK(result->getStatus() ==
          insura::domain::Client::ClientStatus::CONTACTED);
  }

  SECTION("remove, save, reload, client is gone") {
    std::string uuid;
    {
      insura::data::CsvClientRepository repo(tmp.string());

      insura::domain::Client client(
          "Brian", "Kernighan", "bwk@bell-labs.com", std::nullopt, std::nullopt,
          std::nullopt, std::nullopt, std::nullopt, std::nullopt,
          insura::domain::Client::ClientStatus::NEW, std::nullopt);

      uuid = client.getUuid();
      repo.insertClient(client);
      repo.save();
    }

    {
      insura::data::CsvClientRepository repo(tmp.string());
      repo.load();
      REQUIRE(repo.findAll().size() == 1);
      repo.removeClient(uuid);
      repo.save();
    }

    insura::data::CsvClientRepository repo3(tmp.string());
    repo3.load();
    CHECK(repo3.findAll().empty());
    CHECK_FALSE(repo3.findByUuid(uuid).has_value());
  }

  std::filesystem::remove(tmp);
}

TEST_CASE("Policy CSV round-trip preserves data", "[integration]") {
  auto tmp_clients =
      std::filesystem::temp_directory_path() / "test_client_data.csv";
  auto tmp_policies =
      std::filesystem::temp_directory_path() / "test_policy_data.csv";
  std::string client_uuid;

  {
    insura::data::CsvClientRepository client_repo(tmp_clients.string());

    insura::domain::Client client1(
        "John", "Carmack", "jcarmack@doom.edu", std::nullopt,
        std::string("Software Engineer"), std::string("Meta"), std::nullopt,
        std::nullopt, std::nullopt, insura::domain::Client::ClientStatus::NEW,
        std::string("Creator Of Doom"));

    client_repo.insertClient(client1);
    client_uuid = client1.getUuid();

    client_repo.save();
  }

  SECTION(
      "insert one policy with all fields, save, reload, verify all fields") {
    {
      insura::data::CsvPolicyRepository policy_repo(tmp_policies.string());

      insura::domain::Policy policy(
          client_uuid, insura::domain::Policy::PolicyType::HOME, "2026-01-01",
          std::string("2027-01-01"), 1800.00,
          insura::domain::Policy::PolicyStatus::ACTIVE,
          std::string("Full coverage home policy"));

      policy_repo.insertPolicy(policy);
      policy_repo.save();
    }

    insura::data::CsvPolicyRepository repo2(tmp_policies.string());
    repo2.load();
    auto all = repo2.findAll();

    REQUIRE(all.size() == 1);
    const auto& p = all[0];
    CHECK(p.getClientUuid() == client_uuid);
    CHECK(p.getPolicyType() == insura::domain::Policy::PolicyType::HOME);
    CHECK(p.getPolicyStartDate() == "2026-01-01");
    CHECK(p.getPolicyEndDate().value() == "2027-01-01");
    CHECK(p.getPolicyAmount() == Catch::Approx(1800.00));
    CHECK(p.getPolicyStatus() == insura::domain::Policy::PolicyStatus::ACTIVE);
    CHECK(p.getPolicyNotes().value() == "Full coverage home policy");
  }

  SECTION("insert two policies, save, reload, both are findable by client") {
    {
      insura::data::CsvPolicyRepository repo(tmp_policies.string());

      insura::domain::Policy policy1(
          client_uuid, insura::domain::Policy::PolicyType::HOME, "2026-01-01",
          std::string("2027-01-01"), 1800.00,
          insura::domain::Policy::PolicyStatus::ACTIVE, std::nullopt);

      insura::domain::Policy policy2(
          client_uuid, insura::domain::Policy::PolicyType::AUTO, "2026-03-01",
          std::string("2027-03-01"), 650.00,
          insura::domain::Policy::PolicyStatus::PENDING, std::nullopt);

      repo.insertPolicy(policy1);
      repo.insertPolicy(policy2);
      repo.save();
    }

    insura::data::CsvPolicyRepository repo2(tmp_policies.string());
    repo2.load();

    REQUIRE(repo2.findAll().size() == 2);
    CHECK(repo2.findByClientUuid(client_uuid).size() == 2);
  }

  SECTION("edit status and notes, save, reload, changes are persistent") {
    std::string policy_uuid;
    {
      insura::data::CsvPolicyRepository repo(tmp_policies.string());

      insura::domain::Policy policy(
          client_uuid, insura::domain::Policy::PolicyType::LIFE, "2026-01-01",
          std::nullopt, 280.00,
          insura::domain::Policy::PolicyStatus::PENDING, std::nullopt);

      policy_uuid = policy.getUuid();
      repo.insertPolicy(policy);
      repo.save();
    }

    {
      insura::data::CsvPolicyRepository repo(tmp_policies.string());
      repo.load();

      auto result = repo.findByUuid(policy_uuid);
      REQUIRE(result.has_value());

      auto updated = result.value();
      updated.setPolicyStatus(insura::domain::Policy::PolicyStatus::ACTIVE);
      updated.setPolicyNotes("Renewed for another year");
      repo.updatePolicy(updated);
      repo.save();
    }

    insura::data::CsvPolicyRepository repo3(tmp_policies.string());
    repo3.load();

    auto result = repo3.findByUuid(policy_uuid);
    REQUIRE(result.has_value());
    CHECK(result->getPolicyStatus() ==
          insura::domain::Policy::PolicyStatus::ACTIVE);
    CHECK(result->getPolicyNotes().value() == "Renewed for another year");
  }

  SECTION("remove policy, save, reload, policy is gone") {
    std::string policy_uuid;
    {
      insura::data::CsvPolicyRepository repo(tmp_policies.string());

      insura::domain::Policy policy(
          client_uuid, insura::domain::Policy::PolicyType::HEALTH,
          "2026-01-01", std::nullopt, 380.00,
          insura::domain::Policy::PolicyStatus::ACTIVE, std::nullopt);

      policy_uuid = policy.getUuid();
      repo.insertPolicy(policy);
      repo.save();
    }

    {
      insura::data::CsvPolicyRepository repo(tmp_policies.string());
      repo.load();
      REQUIRE(repo.findAll().size() == 1);
      repo.removePolicy(policy_uuid);
      repo.save();
    }

    insura::data::CsvPolicyRepository repo3(tmp_policies.string());
    repo3.load();
    CHECK(repo3.findAll().empty());
    CHECK_FALSE(repo3.findByUuid(policy_uuid).has_value());
  }

  std::filesystem::remove(tmp_clients);
  std::filesystem::remove(tmp_policies);
}

TEST_CASE("Interaction CSV round-trip preserves data", "[integration]") {
  auto tmp_clients =
      std::filesystem::temp_directory_path() / "test_interaction_client.csv";
  auto tmp_interactions =
      std::filesystem::temp_directory_path() / "test_interaction_data.csv";
  std::string client_uuid;

  {
    insura::data::CsvClientRepository client_repo(tmp_clients.string());

    insura::domain::Client client(
        "Ada", "Lovelace", "ada@babbage.io", std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        insura::domain::Client::ClientStatus::NEW, std::nullopt);

    client_uuid = client.getUuid();
    client_repo.insertClient(client);
    client_repo.save();
  }

  SECTION(
      "insert Appointment, save, reload, fields correct and derived type is "
      "Appointment") {
    {
      insura::data::CsvInteractionRepository repo(tmp_interactions.string());

      auto appt = std::make_unique<insura::domain::Appointment>(
          client_uuid, "2026-06-15", "10:00", 60, std::nullopt,
          std::string("Initial policy review meeting"));

      repo.insertInteraction(std::move(appt));
      repo.save();
    }

    insura::data::CsvInteractionRepository repo2(tmp_interactions.string());
    repo2.load();
    auto all = repo2.findAll();

    REQUIRE(all.size() == 1);
    const auto& base = all[0];
    CHECK(base->getClientUuid() == client_uuid);
    CHECK(base->getType() ==
          insura::domain::Interaction::InteractionType::APPOINTMENT);
    CHECK(base->getDate() == "2026-06-15");
    CHECK(base->getNotes().value() == "Initial policy review meeting");

    auto* appt = dynamic_cast<insura::domain::Appointment*>(base.get());
    REQUIRE(appt != nullptr);
    CHECK(appt->getAppointmentTime() == "10:00");
    CHECK(appt->getDuration() == 60);
    CHECK_FALSE(appt->getAppointmentReport().has_value());
  }

  SECTION(
      "insert Contract, save, reload, fields correct and derived type is "
      "Contract") {
    {
      insura::data::CsvInteractionRepository repo(tmp_interactions.string());

      auto contract = std::make_unique<insura::domain::Contract>(
          client_uuid, "2026-06-01", 2400.00, "Home Insurance Plan A",
          "2026-06-01", std::string("2027-06-01"),
          insura::domain::Contract::ContractStatus::ACTIVE, std::nullopt);

      repo.insertInteraction(std::move(contract));
      repo.save();
    }

    insura::data::CsvInteractionRepository repo2(tmp_interactions.string());
    repo2.load();
    auto all = repo2.findAll();

    REQUIRE(all.size() == 1);
    const auto& base = all[0];
    CHECK(base->getClientUuid() == client_uuid);
    CHECK(base->getType() ==
          insura::domain::Interaction::InteractionType::CONTRACT);
    CHECK(base->getDate() == "2026-06-01");

    auto* contract = dynamic_cast<insura::domain::Contract*>(base.get());
    REQUIRE(contract != nullptr);
    CHECK(contract->getValue() == Catch::Approx(2400.00));
    CHECK(contract->getProductName() == "Home Insurance Plan A");
    CHECK(contract->getSignedDate() == "2026-06-01");
    CHECK(contract->getExpiredDate().value() == "2027-06-01");
    CHECK(contract->getStatus() ==
          insura::domain::Contract::ContractStatus::ACTIVE);
  }

  std::filesystem::remove(tmp_clients);
  std::filesystem::remove(tmp_interactions);
}

TEST_CASE(
    "ClientService cascade delete removes related policies and interactions",
    "[integration]") {
  auto tmp_clients =
      std::filesystem::temp_directory_path() / "test_cascade_clients.csv";
  auto tmp_policies =
      std::filesystem::temp_directory_path() / "test_cascade_policies.csv";
  auto tmp_interactions =
      std::filesystem::temp_directory_path() / "test_cascade_interactions.csv";

  insura::data::CsvClientRepository client_repo(tmp_clients.string());
  insura::data::CsvPolicyRepository policy_repo(tmp_policies.string());
  insura::data::CsvInteractionRepository interaction_repo(
      tmp_interactions.string());
  insura::service::ClientService client_service(client_repo, policy_repo,
                                                interaction_repo);

  insura::domain::ClientData client_data;
  client_data.first_name = "John";
  client_data.last_name = "Carmack";
  client_data.email = "jcarmack@doom.edu";
  client_service.addClient(client_data);

  auto all_clients = client_repo.findAll();
  REQUIRE(all_clients.size() == 1);
  std::string client_uuid = all_clients[0].getUuid();

  insura::domain::Policy policy(
      client_uuid, insura::domain::Policy::PolicyType::HOME, "2026-01-01",
      std::string("2027-01-01"), 1800.00,
      insura::domain::Policy::PolicyStatus::ACTIVE, std::nullopt);
  policy_repo.insertPolicy(policy);

  auto appt = std::make_unique<insura::domain::Appointment>(
      client_uuid, "2026-06-15", "10:00", 60, std::nullopt, std::nullopt);
  interaction_repo.insertInteraction(std::move(appt));

  REQUIRE(policy_repo.findByClientUuid(client_uuid).size() == 1);
  REQUIRE(interaction_repo.findByClientUuid(client_uuid).size() == 1);

  client_service.deleteClient(client_uuid);

  CHECK_FALSE(client_repo.findByUuid(client_uuid).has_value());
  CHECK(policy_repo.findByClientUuid(client_uuid).empty());
  CHECK(interaction_repo.findByClientUuid(client_uuid).empty());

  std::filesystem::remove(tmp_clients);
  std::filesystem::remove(tmp_policies);
  std::filesystem::remove(tmp_interactions);
}

TEST_CASE("ClientService rejects duplicate email", "[integration]") {
  auto tmp_clients =
      std::filesystem::temp_directory_path() / "test_dedup_clients.csv";
  auto tmp_policies =
      std::filesystem::temp_directory_path() / "test_dedup_policies.csv";
  auto tmp_interactions =
      std::filesystem::temp_directory_path() / "test_dedup_interactions.csv";

  insura::data::CsvClientRepository client_repo(tmp_clients.string());
  insura::data::CsvPolicyRepository policy_repo(tmp_policies.string());
  insura::data::CsvInteractionRepository interaction_repo(
      tmp_interactions.string());
  insura::service::ClientService client_service(client_repo, policy_repo,
                                                interaction_repo);

  insura::domain::ClientData first;
  first.first_name = "John";
  first.last_name = "Carmack";
  first.email = "jcarmack@doom.edu";

  insura::domain::ClientData second;
  second.first_name = "Jane";
  second.last_name = "Carmack";
  second.email = "jcarmack@doom.edu";

  client_service.addClient(first);
  REQUIRE_THROWS_AS(client_service.addClient(second), std::invalid_argument);
  CHECK(client_repo.findAll().size() == 1);

  std::filesystem::remove(tmp_clients);
  std::filesystem::remove(tmp_policies);
  std::filesystem::remove(tmp_interactions);
}
