#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "../src/data/csv_client_repository.hpp"
#include "../src/domain/client.hpp"

/*
 * Fake data reference. Copy these snippets when writing new tests.
 * Add the relevant includes when needed:
 *   #include "../src/domain/policy.hpp"
 *   #include "../src/domain/interaction.hpp"
 *
 * --- Fake Appointment ---
 *
 *   insura::domain::Appointment appointment(
 *       client1.getUuid(),
 *       "2026-06-15",
 *       "10:00",
 *       60,
 *       std::nullopt,
 *       std::string("Initial policy review meeting")
 *   );
 *
 * --- Fake Contract ---
 *
 *   insura::domain::Contract contract(
 *       client1.getUuid(),
 *       "2026-06-01",
 *       2400.00,
 *       "Home Insurance Plan A",
 *       "2026-06-01",
 *       std::string("2027-06-01"),
 *       insura::domain::Contract::ContractStatus::ACTIVE,
 *       std::nullopt
 *   );
 *
 * --- Fake Policy ---
 *
 *   insura::domain::Policy policy(
 *       client1.getUuid(),
 *       insura::domain::Policy::PolicyType::HOME,
 *       "2026-01-01",
 *       std::string("2027-01-01"),
 *       1800.00,
 *       insura::domain::Policy::PolicyStatus::ACTIVE,
 *       std::nullopt
 *   );
 */

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

  SECTION("insert one client with all fields, save, reload, verify all fields") {
    {
      insura::data::CsvClientRepository repo(tmp.string());

      insura::domain::Client client(
          "Ada", "Lovelace", "ada@babbage.io",
          std::string("+39 02 1234567"),
          std::string("Mathematician"),
          std::string("Analytical Engine Co."),
          std::string("10 Downing Street"),
          std::string("London"),
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
          std::string("Rear Admiral"), std::nullopt, std::nullopt,
          std::nullopt, std::nullopt,
          insura::domain::Client::ClientStatus::NEW, std::nullopt);

      insura::domain::Client client2(
          "Alan", "Turing", "aturing@bletchley.uk", std::nullopt,
          std::string("Cryptanalyst"), std::nullopt, std::nullopt,
          std::nullopt, std::nullopt,
          insura::domain::Client::ClientStatus::NEW, std::nullopt);

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
          std::nullopt, std::nullopt,
          insura::domain::Client::ClientStatus::NEW, std::nullopt);

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
    CHECK(result->getStatus() == insura::domain::Client::ClientStatus::CONTACTED);
  }

  SECTION("remove, save, reload, client is gone") {
    std::string uuid;
    {
      insura::data::CsvClientRepository repo(tmp.string());

      insura::domain::Client client(
          "Brian", "Kernighan", "bwk@bell-labs.com", std::nullopt,
          std::nullopt, std::nullopt, std::nullopt, std::nullopt,
          std::nullopt, insura::domain::Client::ClientStatus::NEW,
          std::nullopt);

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
